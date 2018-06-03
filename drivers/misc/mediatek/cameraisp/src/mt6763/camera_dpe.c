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
#include "smi_public.h"
/*#include <linux/xlog.h>		 For xlog_printk(). */
/*  */
/*#include <mach/hardware.h>*/
/* #include <mach/mt6593_pll.h> */
#include "inc/camera_dpe.h"
/*#include <mach/irqs.h>*/
/* #include <mach/mt_reg_base.h> */

#include <mt-plat/sync_write.h>	/* For mt65xx_reg_sync_writel(). */
/* #include <mach/mt_spm_idle.h>	 For spm_enable_sodi()/spm_disable_sodi(). */

#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <m4u.h>
#include <cmdq_core.h>
#include <cmdq_record.h>

/* #define __DPE_EP_NO_CLKMGR__ */

/* Measure the kernel performance */
/* #define __DPE_KERNEL_PERFORMANCE_MEASURE__ */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
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
	event_trace_printk(tracing_mark_write_addr,  "B|%d|%s\n", current->tgid, name);\
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

typedef unsigned char MUINT8;
typedef unsigned int MUINT32;
/*  */
typedef signed char MINT8;

/* DPE Command Queue */
/* #include "../../cmdq/mt6797/cmdq_record.h" */
/* #include "../../cmdq/mt6797/cmdq_core.h" */

/* CCF */
#ifndef __DPE_EP_NO_CLKMGR__
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#include <linux/clk.h>
typedef struct{
	struct clk *CG_IMGSYS_DPE;
} DPE_CLK_STRUCT;
DPE_CLK_STRUCT dpe_clk;
#endif				/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
#endif
typedef signed int MINT32;
/*  */
typedef bool MBOOL;
/*  */
#ifndef MTRUE
#define MTRUE               1
#endif
#ifndef MFALSE
#define MFALSE              0
#endif

#define DPE_DEV_NAME                "camera-dpe"

/* #define DPE_WAITIRQ_LOG  */
#define DPE_USE_GCE
/* #define DPE_DEBUG_USE  */
/* #define DPE_MULTIPROCESS_TIMEING_ISSUE  */
/*I can' test the situation in FPGA, because the velocity of FPGA is so slow. */
#define MyTag "[DPE]"
#define IRQTag "KEEPER"

#define LOG_VRB(format,	args...)    pr_debug(MyTag format, ##args)

#ifdef DPE_DEBUG_USE
#define LOG_DBG(format, args...)    pr_debug(MyTag format, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...)    pr_debug(MyTag format,  ##args)
#define LOG_NOTICE(format, args...) pr_notice(MyTag format,  ##args)
#define LOG_WRN(format, args...)    pr_warn(MyTag format,  ##args)
#define LOG_ERR(format, args...)    pr_err(MyTag format,  ##args)
#define LOG_AST(format, args...)    pr_alert(MyTag format, ##args)


/*******************************************************************************
*
********************************************************************************/
/* #define DPE_WR32(addr, data)    iowrite32(data, addr) // For other projects. */
#define DPE_WR32(addr, data)    mt_reg_sync_writel(data, addr)	/* For 89 Only.   // NEED_TUNING_BY_PROJECT */
#define DPE_RD32(addr)          ioread32(addr)
#define DPE_SET_BIT(reg, bit)   ((*(volatile MUINT32 *)(reg)) |= (MUINT32)(1 << (bit)))
#define DPE_CLR_BIT(reg, bit)   ((*(volatile MUINT32 *)(reg)) &= ~((MUINT32)(1 << (bit))))
/*******************************************************************************
*
********************************************************************************/
/* dynamic log level */
#define DPE_DBG_DBGLOG              (0x00000001)
#define DPE_DBG_INFLOG              (0x00000002)
#define DPE_DBG_INT                 (0x00000004)
#define DPE_DBG_READ_REG            (0x00000008)
#define DPE_DBG_WRITE_REG           (0x00000010)
#define DPE_DBG_TASKLET             (0x00000020)


/* ///////////////////////////////////////////////////////////////// */

/*******************************************************************************
*
********************************************************************************/

/* CAM interrupt status */
/* normal siganl */
#define DPE_INT_ST           (1<<0)
#define DVE_INT_ST           (1<<0)
#define WMFE_INT_ST          (1<<0)


/* IRQ signal mask */

#define INT_ST_MASK_DPE     ( \
			DPE_INT_ST)

#define CMDQ_REG_MASK 0xffffffff
#define DVE_START       0x1
#define WMFE_START      0x1

#define WMFE_ENABLE     0x1

#define DVE_IS_BUSY     0x1
#define WMFE_IS_BUSY    0x2


/* static irqreturn_t DPE_Irq_CAM_A(MINT32  Irq,void *DeviceId); */
static irqreturn_t ISP_Irq_DPE(MINT32 Irq, void *DeviceId);
static MINT32 ConfigDVEHW(DPE_DVEConfig *pDveConfig);
static MINT32 ConfigWMFEHW(DPE_WMFEConfig *pWmfeConfig);
static void DPE_ScheduleDveWork(struct work_struct *data);
static void DPE_ScheduleWmfeWork(struct work_struct *data);



typedef irqreturn_t(*IRQ_CB) (MINT32, void *);

typedef struct {
	IRQ_CB isr_fp;
	unsigned int int_number;
	char device_name[16];
} ISR_TABLE;

#ifndef CONFIG_OF
const ISR_TABLE DPE_IRQ_CB_TBL[DPE_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_DPE, DPE_IRQ_BIT_ID, "dpe"},
};

#else
/* int number is got from kernel api */
const ISR_TABLE DPE_IRQ_CB_TBL[DPE_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_DPE, 0, "dpe"},
};

#endif
/* //////////////////////////////////////////////////////////////////////////////////////////// */
/*  */
typedef void (*tasklet_cb) (unsigned long);
typedef struct {
	tasklet_cb tkt_cb;
	struct tasklet_struct *pDPE_tkt;
} Tasklet_table;

struct tasklet_struct Dpetkt[DPE_IRQ_TYPE_AMOUNT];

static void ISP_TaskletFunc_DPE(unsigned long data);

static Tasklet_table DPE_tasklet[DPE_IRQ_TYPE_AMOUNT] = {
	{ISP_TaskletFunc_DPE, &Dpetkt[DPE_IRQ_TYPE_INT_DPE_ST]},
};

struct wake_lock DPE_wake_lock;

static DEFINE_MUTEX(gDpeDveMutex);
static DEFINE_MUTEX(gDpeDveDequeMutex);

static DEFINE_MUTEX(gDpeWmfeMutex);
static DEFINE_MUTEX(gDpeWmfeDequeMutex);

#ifdef CONFIG_OF

struct DPE_device {
	void __iomem *regs;
	struct device *dev;
	int irq;
};

static struct DPE_device *DPE_devs;
static int nr_DPE_devs;

/* Get HW modules' base address from device nodes */
#define DPE_DEV_NODE_IDX 0

/* static unsigned long gISPSYS_Reg[DPE_IRQ_TYPE_AMOUNT]; */


#define ISP_DPE_BASE                  (DPE_devs[DPE_DEV_NODE_IDX].regs)
/* #define ISP_DPE_BASE                  (gISPSYS_Reg[DPE_DEV_NODE_IDX]) */



#else
#define ISP_DPE_BASE                        (IMGSYS_BASE + 0x2800)

#endif


static unsigned int g_u4EnableClockCount;

/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32


typedef enum {
	DPE_FRAME_STATUS_EMPTY,	/* 0 */
	DPE_FRAME_STATUS_ENQUE,	/* 1 */
	DPE_FRAME_STATUS_RUNNING,	/* 2 */
	DPE_FRAME_STATUS_FINISHED,	/* 3 */
	DPE_FRAME_STATUS_TOTAL
} DPE_FRAME_STATUS_ENUM;


typedef enum {
	DPE_REQUEST_STATE_EMPTY,	/* 0 */
	DPE_REQUEST_STATE_PENDING,	/* 1 */
	DPE_REQUEST_STATE_RUNNING,	/* 2 */
	DPE_REQUEST_STATE_FINISHED,	/* 3 */
	DPE_REQUEST_STATE_TOTAL
} DPE_REQUEST_STATE_ENUM;


typedef struct {
	DPE_REQUEST_STATE_ENUM RequestState;
	pid_t processID;	/* caller process ID */
	MUINT32 callerID;	/* caller thread ID */
	MUINT32 enqueReqNum;	/* to judge it belongs to which frame package */
	MINT32 FrameWRIdx;	/* Frame write Index */
	MINT32 FrameRDIdx;	/* Frame read Index */
	DPE_FRAME_STATUS_ENUM DveFrameStatus[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
	DPE_DVEConfig DveFrameConfig[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
} DVE_REQUEST_STRUCT;

typedef struct {
	MINT32 WriteIdx;	/* enque how many request  */
	MINT32 ReadIdx;		/* read which request index */
	MINT32 HWProcessIdx;	/* HWWriteIdx */
	DVE_REQUEST_STRUCT DVEReq_Struct[_SUPPORT_MAX_DPE_REQUEST_RING_SIZE_];
} DVE_REQUEST_RING_STRUCT;

typedef struct {
	DPE_DVEConfig DveFrameConfig[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
} DVE_CONFIG_STRUCT;


static DVE_REQUEST_RING_STRUCT g_DVE_RequestRing;
static DVE_CONFIG_STRUCT g_DveEnqueReq_Struct;
static DVE_CONFIG_STRUCT g_DveDequeReq_Struct;


typedef struct {
	DPE_REQUEST_STATE_ENUM RequestState;
	pid_t processID;	/* caller process ID */
	MUINT32 callerID;	/* caller thread ID */
	MUINT32 enqueReqNum;	/* to judge it belongs to which frame package */
	MINT32 FrameWRIdx;	/* Frame write Index */
	MINT32 FrameRDIdx;	/* Frame read Index */
	DPE_FRAME_STATUS_ENUM WmfeFrameStatus[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
	DPE_WMFEConfig WmfeFrameConfig[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
} WMFE_REQUEST_STRUCT;

typedef struct {
	MINT32 WriteIdx;	/* enque how many request  */
	MINT32 ReadIdx;		/* read which request index */
	MINT32 HWProcessIdx;	/* HWWriteIdx */
	WMFE_REQUEST_STRUCT WMFEReq_Struct[_SUPPORT_MAX_DPE_REQUEST_RING_SIZE_];
} WMFE_REQUEST_RING_STRUCT;

typedef struct {
	DPE_WMFEConfig WmfeFrameConfig[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
} WMFE_CONFIG_STRUCT;

static WMFE_REQUEST_RING_STRUCT g_WMFE_ReqRing;
static WMFE_CONFIG_STRUCT g_WmfeEnqueReq_Struct;
static WMFE_CONFIG_STRUCT g_WmfeDequeReq_Struct;


/*******************************************************************************
*
********************************************************************************/
typedef struct {
	pid_t Pid;
	pid_t Tid;
} DPE_USER_INFO_STRUCT;
typedef enum {
	DPE_PROCESS_ID_NONE,
	DPE_PROCESS_ID_DVE,
	DPE_PROCESS_ID_WMFE,
	DPE_PROCESS_ID_AMOUNT
} DPE_PROCESS_ID_ENUM;


/*******************************************************************************
*
********************************************************************************/
typedef struct {
	volatile MUINT32 Status[DPE_IRQ_TYPE_AMOUNT];
	MINT32 DveIrqCnt;
	MINT32 WmfeIrqCnt;
	pid_t ProcessID[DPE_PROCESS_ID_AMOUNT];
	MUINT32 Mask[DPE_IRQ_TYPE_AMOUNT];
} DPE_IRQ_INFO_STRUCT;


typedef struct {
	spinlock_t SpinLockDPERef;
	spinlock_t SpinLockDPE;
	spinlock_t SpinLockIrq[DPE_IRQ_TYPE_AMOUNT];
	wait_queue_head_t WaitQueueHead;
	struct work_struct ScheduleDveWork;
	struct work_struct ScheduleWmfeWork;
	MUINT32 UserCount;	/* User Count */
	MUINT32 DebugMask;	/* Debug Mask */
	MINT32 IrqNum;
	DPE_IRQ_INFO_STRUCT IrqInfo;
	MINT32 WriteReqIdx;
	MINT32 ReadReqIdx;
	pid_t ProcessID[_SUPPORT_MAX_DPE_FRAME_REQUEST_];
} DPE_INFO_STRUCT;


static DPE_INFO_STRUCT DPEInfo;

typedef enum _eLOG_TYPE {
	_LOG_DBG = 0,		/* currently, only used at ipl_buf_ctrl. to protect critical section */
	_LOG_INF = 1,
	_LOG_ERR = 2,
	_LOG_MAX = 3,
} eLOG_TYPE;

#define NORMAL_STR_LEN (512)
#define ERR_PAGE 2
#define DBG_PAGE 2
#define INF_PAGE 4
/* #define SV_LOG_STR_LEN NORMAL_STR_LEN */

#define LOG_PPNUM 2
static MUINT32 m_CurrentPPB;
typedef struct _SV_LOG_STR {
	MUINT32 _cnt[LOG_PPNUM][_LOG_MAX];
	/* char   _str[_LOG_MAX][SV_LOG_STR_LEN]; */
	char *_str[LOG_PPNUM][_LOG_MAX];
} SV_LOG_STR, *PSV_LOG_STR;

static void *pLog_kmalloc;
static SV_LOG_STR gSvLog[DPE_IRQ_TYPE_AMOUNT];

/* for irq used,keep log until IRQ_LOG_PRINTER being involked, */
/*    limited: */
/*    each log must shorter than 512 bytes */
/*    total log length in each irq/logtype can't over 1024 bytes */

#if 1
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...) do {\
	char *ptr; \
	char *pDes;\
	MUINT32 *ptr2 = &gSvLog[irq]._cnt[ppb][logT];\
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
	ptr = pDes = (char *)&(gSvLog[irq]._str[ppb][logT][gSvLog[irq]._cnt[ppb][logT]]);    \
	sprintf((char *)(pDes), fmt, ##__VA_ARGS__);   \
	if ('\0' != gSvLog[irq]._str[ppb][logT][str_leng - 1]) {\
		LOG_ERR("log str over flow(%d)", irq);\
	} \
	while (*ptr++ != '\0') {        \
		(*ptr2)++;\
	}     \
} while (0)
#endif

#if 1
#define IRQ_LOG_PRINTER(irq, ppb_in, logT_in) do {\
	SV_LOG_STR *pSrc = &gSvLog[irq];\
	char *ptr;\
	MUINT32 i;\
	MINT32 ppb = 0;\
	MINT32 logT = 0;\
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


/* DPE registers */
#define DPE_RST_HW                    (DPE_BASE_HW)
#define DPE_INT_CTL_HW                (DPE_BASE_HW + 0x08)
#define DPE_INT_STATUS_HW             (DPE_BASE_HW + 0x18)
#define DPE_DBG_INFO_0_HW             (DPE_BASE_HW + 0x1C)

#define DPE_DVE_START_HW              (DPE_BASE_HW + 0x20)
#define DPE_DVE_INT_CTRL_HW           (DPE_BASE_HW + 0x24)
#define DPE_DVE_INT_STATUS_HW         (DPE_BASE_HW + 0x28)

#define DPE_DVE_CTRL_HW               (DPE_BASE_HW + 0x2C)
#define DPE_DVE_ORG_L_HORZ_BBOX_HW    (DPE_BASE_HW + 0x30)
#define DPE_DVE_ORG_L_VERT_BBOX_HW    (DPE_BASE_HW + 0x34)
#define DPE_DVE_ORG_R_HORZ_BBOX_HW    (DPE_BASE_HW + 0x38)
#define DPE_DVE_ORG_R_VERT_BBOX_HW    (DPE_BASE_HW + 0x3C)
#define DPE_DVE_ORG_SIZE_HW           (DPE_BASE_HW + 0x40)
#define DPE_DVE_ORG_SR_0_HW           (DPE_BASE_HW + 0x44)
#define DPE_DVE_ORG_SR_1_HW           (DPE_BASE_HW + 0x48)
#define DPE_DVE_ORG_SV_HW             (DPE_BASE_HW + 0x4C)

#define DPE_DVE_CAND_NUM_HW           (DPE_BASE_HW + 0x50)
#define DPE_DVE_CAND_SEL_0_HW         (DPE_BASE_HW + 0x54)
#define DPE_DVE_CAND_SEL_1_HW         (DPE_BASE_HW + 0x58)
#define DPE_DVE_CAND_SEL_2_HW         (DPE_BASE_HW + 0x5C)
#define DPE_DVE_CAND_TYPE_0_HW        (DPE_BASE_HW + 0x60)
#define DPE_DVE_CAND_TYPE_1_HW        (DPE_BASE_HW + 0x64)
#define DPE_DVE_RAND_LUT_HW           (DPE_BASE_HW + 0x68)
#define DPE_DVE_GMV_HW                (DPE_BASE_HW + 0x6C)
#define DPE_DVE_DV_INI_HW             (DPE_BASE_HW + 0x70)

#define DPE_DVE_BLK_VAR_CTRL_HW       (DPE_BASE_HW + 0x74)
#define DPE_DVE_SMTH_LUMA_CTRL_HW     (DPE_BASE_HW + 0x78)
#define DPE_DVE_SMTH_DV_CTRL_HW       (DPE_BASE_HW + 0x7C)
#define DPE_DVE_ORD_CTRL_HW           (DPE_BASE_HW + 0x80)
#define DPE_DVE_TYPE_CTRL_0_HW        (DPE_BASE_HW + 0x84)
#define DPE_DVE_TYPE_CTRL_1_HW        (DPE_BASE_HW + 0x88)

#define DPE_DVE_IMGI_L_BASE_ADDR_HW   (DPE_BASE_HW + 0x8C)
#define DPE_DVE_IMGI_L_STRIDE_HW      (DPE_BASE_HW + 0x90)
#define DPE_DVE_IMGI_R_BASE_ADDR_HW   (DPE_BASE_HW + 0x94)
#define DPE_DVE_IMGI_R_STRIDE_HW      (DPE_BASE_HW + 0x98)

#define DPE_DVE_DVI_L_BASE_ADDR_HW    (DPE_BASE_HW + 0x9C)
#define DPE_DVE_DVI_L_STRIDE_HW       (DPE_BASE_HW + 0xA0)
#define DPE_DVE_DVI_R_BASE_ADDR_HW    (DPE_BASE_HW + 0xA4)
#define DPE_DVE_DVI_R_STRIDE_HW       (DPE_BASE_HW + 0xA8)

#define DPE_DVE_MASKI_L_BASE_ADDR_HW  (DPE_BASE_HW + 0xAC)
#define DPE_DVE_MASKI_L_STRIDE_HW     (DPE_BASE_HW + 0xB0)
#define DPE_DVE_MASKI_R_BASE_ADDR_HW  (DPE_BASE_HW + 0xB4)
#define DPE_DVE_MASKI_R_STRIDE_HW     (DPE_BASE_HW + 0xB8)

#define DPE_DVE_DVO_L_BASE_ADDR_HW    (DPE_BASE_HW + 0xBC)
#define DPE_DVE_DVO_L_STRIDE_HW       (DPE_BASE_HW + 0xC0)
#define DPE_DVE_DVO_R_BASE_ADDR_HW    (DPE_BASE_HW + 0xC4)
#define DPE_DVE_DVO_R_STRIDE_HW       (DPE_BASE_HW + 0xC8)

#define DPE_DVE_CONFO_L_BASE_ADDR_HW  (DPE_BASE_HW + 0xCC)
#define DPE_DVE_CONFO_L_STRIDE_HW     (DPE_BASE_HW + 0xD0)
#define DPE_DVE_CONFO_R_BASE_ADDR_HW  (DPE_BASE_HW + 0xD4)
#define DPE_DVE_CONFO_R_STRIDE_HW     (DPE_BASE_HW + 0xD8)

#define DPE_DVE_RESPO_L_BASE_ADDR_HW  (DPE_BASE_HW + 0xDC)
#define DPE_DVE_RESPO_L_STRIDE_HW     (DPE_BASE_HW + 0xE0)
#define DPE_DVE_RESPO_R_BASE_ADDR_HW  (DPE_BASE_HW + 0xE4)
#define DPE_DVE_RESPO_R_STRIDE_HW     (DPE_BASE_HW + 0xE8)

#define DPE_DVE_TYPE_CTRL_2_HW        (DPE_BASE_HW + 0xEC)

#define DPE_DVE_STA_HW                (DPE_BASE_HW + 0x100)
#define DPE_DVE_DBG_INFO_00_HW        (DPE_BASE_HW + 0x120)
#define DPE_DVE_DBG_INFO_01_HW        (DPE_BASE_HW + 0x124)
#define DPE_DVE_DBG_INFO_02_HW        (DPE_BASE_HW + 0x128)
#define DPE_DVE_DBG_INFO_03_HW        (DPE_BASE_HW + 0x12C)
#define DPE_DVE_DBG_INFO_04_HW        (DPE_BASE_HW + 0x130)
#define DPE_DVE_DBG_INFO_05_HW        (DPE_BASE_HW + 0x134)
#define DPE_DVE_DBG_INFO_06_HW        (DPE_BASE_HW + 0x138)
#define DPE_DVE_DBG_INFO_07_HW        (DPE_BASE_HW + 0x13C)
#define DPE_DVE_DBG_INFO_08_HW        (DPE_BASE_HW + 0x140)
#define DPE_DVE_DBG_INFO_09_HW        (DPE_BASE_HW + 0x144)

#define DPE_WMFE_START_HW             (DPE_BASE_HW + 0x220)
#define DPE_WMFE_INT_CTRL_HW          (DPE_BASE_HW + 0x224)
#define DPE_WMFE_INT_STATUS_HW        (DPE_BASE_HW + 0x228)

#define DPE_WMFE_CTRL_0_HW            (DPE_BASE_HW + 0x230)
#define DPE_WMFE_SIZE_0_HW            (DPE_BASE_HW + 0x234)
#define DPE_WMFE_IMGI_BASE_ADDR_0_HW  (DPE_BASE_HW + 0x238)
#define DPE_WMFE_IMGI_STRIDE_0_HW     (DPE_BASE_HW + 0x23C)
#define DPE_WMFE_DPI_BASE_ADDR_0_HW   (DPE_BASE_HW + 0x240)
#define DPE_WMFE_DPI_STRIDE_0_HW      (DPE_BASE_HW + 0x244)
#define DPE_WMFE_TBLI_BASE_ADDR_0_HW  (DPE_BASE_HW + 0x248)
#define DPE_WMFE_TBLI_STRIDE_0_HW     (DPE_BASE_HW + 0x24C)
#define DPE_WMFE_MASKI_BASE_ADDR_0_HW (DPE_BASE_HW + 0x250)
#define DPE_WMFE_MASKI_STRIDE_0_HW    (DPE_BASE_HW + 0x254)
#define DPE_WMFE_DPO_BASE_ADDR_0_HW   (DPE_BASE_HW + 0x258)
#define DPE_WMFE_DPO_STRIDE_0_HW      (DPE_BASE_HW + 0x25C)


#define DPE_WMFE_CTRL_1_HW            (DPE_BASE_HW + 0x270)
#define DPE_WMFE_SIZE_1_HW            (DPE_BASE_HW + 0x274)
#define DPE_WMFE_IMGI_BASE_ADDR_1_HW  (DPE_BASE_HW + 0x278)
#define DPE_WMFE_IMGI_STRIDE_1_HW     (DPE_BASE_HW + 0x27C)
#define DPE_WMFE_DPI_BASE_ADDR_1_HW   (DPE_BASE_HW + 0x280)
#define DPE_WMFE_DPI_STRIDE_1_HW      (DPE_BASE_HW + 0x284)
#define DPE_WMFE_TBLI_BASE_ADDR_1_HW  (DPE_BASE_HW + 0x288)
#define DPE_WMFE_TBLI_STRIDE_1_HW     (DPE_BASE_HW + 0x28C)
#define DPE_WMFE_MASKI_BASE_ADDR_1_HW (DPE_BASE_HW + 0x290)
#define DPE_WMFE_MASKI_STRIDE_1_HW    (DPE_BASE_HW + 0x294)
#define DPE_WMFE_DPO_BASE_ADDR_1_HW   (DPE_BASE_HW + 0x298)
#define DPE_WMFE_DPO_STRIDE_1_HW      (DPE_BASE_HW + 0x29C)

#define DPE_WMFE_CTRL_2_HW            (DPE_BASE_HW + 0x2B0)
#define DPE_WMFE_SIZE_2_HW            (DPE_BASE_HW + 0x2B4)
#define DPE_WMFE_IMGI_BASE_ADDR_2_HW  (DPE_BASE_HW + 0x2B8)
#define DPE_WMFE_IMGI_STRIDE_2_HW     (DPE_BASE_HW + 0x2BC)
#define DPE_WMFE_DPI_BASE_ADDR_2_HW   (DPE_BASE_HW + 0x2C0)
#define DPE_WMFE_DPI_STRIDE_2_HW      (DPE_BASE_HW + 0x2C4)
#define DPE_WMFE_TBLI_BASE_ADDR_2_HW  (DPE_BASE_HW + 0x2C8)
#define DPE_WMFE_TBLI_STRIDE_2_HW     (DPE_BASE_HW + 0x2CC)
#define DPE_WMFE_MASKI_BASE_ADDR_2_HW (DPE_BASE_HW + 0x2D0)
#define DPE_WMFE_MASKI_STRIDE_2_HW    (DPE_BASE_HW + 0x2D4)
#define DPE_WMFE_DPO_BASE_ADDR_2_HW   (DPE_BASE_HW + 0x2D8)
#define DPE_WMFE_DPO_STRIDE_2_HW      (DPE_BASE_HW + 0x2DC)

#define DPE_WMFE_CTRL_3_HW            (DPE_BASE_HW + 0x2F0)
#define DPE_WMFE_SIZE_3_HW            (DPE_BASE_HW + 0x2F4)
#define DPE_WMFE_IMGI_BASE_ADDR_3_HW  (DPE_BASE_HW + 0x2F8)
#define DPE_WMFE_IMGI_STRIDE_3_HW     (DPE_BASE_HW + 0x2FC)
#define DPE_WMFE_DPI_BASE_ADDR_3_HW   (DPE_BASE_HW + 0x300)
#define DPE_WMFE_DPI_STRIDE_3_HW      (DPE_BASE_HW + 0x304)
#define DPE_WMFE_TBLI_BASE_ADDR_3_HW  (DPE_BASE_HW + 0x308)
#define DPE_WMFE_TBLI_STRIDE_3_HW     (DPE_BASE_HW + 0x30C)
#define DPE_WMFE_MASKI_BASE_ADDR_3_HW (DPE_BASE_HW + 0x310)
#define DPE_WMFE_MASKI_STRIDE_3_HW    (DPE_BASE_HW + 0x314)
#define DPE_WMFE_DPO_BASE_ADDR_3_HW   (DPE_BASE_HW + 0x318)
#define DPE_WMFE_DPO_STRIDE_3_HW      (DPE_BASE_HW + 0x31C)

#define DPE_WMFE_CTRL_4_HW            (DPE_BASE_HW + 0x330)
#define DPE_WMFE_SIZE_4_HW            (DPE_BASE_HW + 0x334)
#define DPE_WMFE_IMGI_BASE_ADDR_4_HW  (DPE_BASE_HW + 0x338)
#define DPE_WMFE_IMGI_STRIDE_4_HW     (DPE_BASE_HW + 0x33C)
#define DPE_WMFE_DPI_BASE_ADDR_4_HW   (DPE_BASE_HW + 0x340)
#define DPE_WMFE_DPI_STRIDE_4_HW      (DPE_BASE_HW + 0x344)
#define DPE_WMFE_TBLI_BASE_ADDR_4_HW  (DPE_BASE_HW + 0x348)
#define DPE_WMFE_TBLI_STRIDE_4_HW     (DPE_BASE_HW + 0x34C)
#define DPE_WMFE_MASKI_BASE_ADDR_4_HW (DPE_BASE_HW + 0x350)
#define DPE_WMFE_MASKI_STRIDE_4_HW    (DPE_BASE_HW + 0x354)
#define DPE_WMFE_DPO_BASE_ADDR_4_HW   (DPE_BASE_HW + 0x358)
#define DPE_WMFE_DPO_STRIDE_4_HW      (DPE_BASE_HW + 0x35C)



#define DPE_WMFE_DBG_INFO_00_HW       (DPE_BASE_HW + 0x400)
#define DPE_WMFE_DBG_INFO_01_HW       (DPE_BASE_HW + 0x404)
#define DPE_WMFE_DBG_INFO_02_HW       (DPE_BASE_HW + 0x408)
#define DPE_WMFE_DBG_INFO_03_HW       (DPE_BASE_HW + 0x40C)
#define DPE_WMFE_DBG_INFO_04_HW       (DPE_BASE_HW + 0x410)
#define DPE_WMFE_DBG_INFO_05_HW       (DPE_BASE_HW + 0x414)
#define DPE_WMFE_DBG_INFO_06_HW       (DPE_BASE_HW + 0x418)
#define DPE_WMFE_DBG_INFO_07_HW       (DPE_BASE_HW + 0x41C)
#define DPE_WMFE_DBG_INFO_08_HW       (DPE_BASE_HW + 0x420)
#define DPE_WMFE_DBG_INFO_09_HW       (DPE_BASE_HW + 0x424)

#define DPE_DMA_DBG_HW                (DPE_BASE_HW + 0x7F4)
#define DPE_DMA_REQ_STATUS_HW         (DPE_BASE_HW + 0x7F8)
#define DPE_DMA_RDY_STATUS_HW         (DPE_BASE_HW + 0x7FC)





#define DPE_RST_REG                    (ISP_DPE_BASE)
#define DPE_INT_CTL_REG                (ISP_DPE_BASE + 0x08)
#define DPE_INT_STATUS_REG             (ISP_DPE_BASE + 0x18)
#define DPE_DBG_INFO_0_REG             (ISP_DPE_BASE + 0x1C)

#define DPE_DVE_START_REG              (ISP_DPE_BASE + 0x20)
#define DPE_DVE_INT_CTRL_REG           (ISP_DPE_BASE + 0x24)
#define DPE_DVE_INT_STATUS_REG         (ISP_DPE_BASE + 0x28)

#define DPE_DVE_CTRL_REG               (ISP_DPE_BASE + 0x2C)
#define DPE_DVE_ORG_L_HORZ_BBOX_REG    (ISP_DPE_BASE + 0x30)
#define DPE_DVE_ORG_L_VERT_BBOX_REG    (ISP_DPE_BASE + 0x34)
#define DPE_DVE_ORG_R_HORZ_BBOX_REG    (ISP_DPE_BASE + 0x38)
#define DPE_DVE_ORG_R_VERT_BBOX_REG    (ISP_DPE_BASE + 0x3C)
#define DPE_DVE_ORG_SIZE_REG           (ISP_DPE_BASE + 0x40)
#define DPE_DVE_ORG_SR_0_REG           (ISP_DPE_BASE + 0x44)
#define DPE_DVE_ORG_SR_1_REG           (ISP_DPE_BASE + 0x48)
#define DPE_DVE_ORG_SV_REG             (ISP_DPE_BASE + 0x4C)

#define DPE_DVE_CAND_NUM_REG           (ISP_DPE_BASE + 0x50)
#define DPE_DVE_CAND_SEL_0_REG         (ISP_DPE_BASE + 0x54)
#define DPE_DVE_CAND_SEL_1_REG         (ISP_DPE_BASE + 0x58)
#define DPE_DVE_CAND_SEL_2_REG         (ISP_DPE_BASE + 0x5C)
#define DPE_DVE_CAND_TYPE_0_REG        (ISP_DPE_BASE + 0x60)
#define DPE_DVE_CAND_TYPE_1_REG        (ISP_DPE_BASE + 0x64)
#define DPE_DVE_RAND_LUT_REG           (ISP_DPE_BASE + 0x68)
#define DPE_DVE_GMV_REG                (ISP_DPE_BASE + 0x6C)
#define DPE_DVE_DV_INI_REG             (ISP_DPE_BASE + 0x70)

#define DPE_DVE_BLK_VAR_CTRL_REG       (ISP_DPE_BASE + 0x74)
#define DPE_DVE_SMTH_LUMA_CTRL_REG     (ISP_DPE_BASE + 0x78)
#define DPE_DVE_SMTH_DV_CTRL_REG       (ISP_DPE_BASE + 0x7C)
#define DPE_DVE_ORD_CTRL_REG           (ISP_DPE_BASE + 0x80)
#define DPE_DVE_TYPE_CTRL_0_REG        (ISP_DPE_BASE + 0x84)
#define DPE_DVE_TYPE_CTRL_1_REG        (ISP_DPE_BASE + 0x88)

#define DPE_DVE_IMGI_L_BASE_ADDR_REG   (ISP_DPE_BASE + 0x8C)
#define DPE_DVE_IMGI_L_STRIDE_REG      (ISP_DPE_BASE + 0x90)
#define DPE_DVE_IMGI_R_BASE_ADDR_REG   (ISP_DPE_BASE + 0x94)
#define DPE_DVE_IMGI_R_STRIDE_REG      (ISP_DPE_BASE + 0x98)

#define DPE_DVE_DVI_L_BASE_ADDR_REG    (ISP_DPE_BASE + 0x9C)
#define DPE_DVE_DVI_L_STRIDE_REG       (ISP_DPE_BASE + 0xA0)
#define DPE_DVE_DVI_R_BASE_ADDR_REG    (ISP_DPE_BASE + 0xA4)
#define DPE_DVE_DVI_R_STRIDE_REG       (ISP_DPE_BASE + 0xA8)

#define DPE_DVE_MASKI_L_BASE_ADDR_REG  (ISP_DPE_BASE + 0xAC)
#define DPE_DVE_MASKI_L_STRIDE_REG     (ISP_DPE_BASE + 0xB0)
#define DPE_DVE_MASKI_R_BASE_ADDR_REG  (ISP_DPE_BASE + 0xB4)
#define DPE_DVE_MASKI_R_STRIDE_REG     (ISP_DPE_BASE + 0xB8)

#define DPE_DVE_DVO_L_BASE_ADDR_REG    (ISP_DPE_BASE + 0xBC)
#define DPE_DVE_DVO_L_STRIDE_REG       (ISP_DPE_BASE + 0xC0)
#define DPE_DVE_DVO_R_BASE_ADDR_REG    (ISP_DPE_BASE + 0xC4)
#define DPE_DVE_DVO_R_STRIDE_REG       (ISP_DPE_BASE + 0xC8)

#define DPE_DVE_CONFO_L_BASE_ADDR_REG  (ISP_DPE_BASE + 0xCC)
#define DPE_DVE_CONFO_L_STRIDE_REG     (ISP_DPE_BASE + 0xD0)
#define DPE_DVE_CONFO_R_BASE_ADDR_REG  (ISP_DPE_BASE + 0xD4)
#define DPE_DVE_CONFO_R_STRIDE_REG     (ISP_DPE_BASE + 0xD8)

#define DPE_DVE_RESPO_L_BASE_ADDR_REG  (ISP_DPE_BASE + 0xDC)
#define DPE_DVE_RESPO_L_STRIDE_REG     (ISP_DPE_BASE + 0xE0)
#define DPE_DVE_RESPO_R_BASE_ADDR_REG  (ISP_DPE_BASE + 0xE4)
#define DPE_DVE_RESPO_R_STRIDE_REG     (ISP_DPE_BASE + 0xE8)

#define DPE_DVE_TYPE_CTRL_2_REG        (ISP_DPE_BASE + 0xEC)


#define DPE_DVE_STA_REG                (ISP_DPE_BASE + 0x100)
#define DPE_DVE_DBG_INFO_00_REG        (ISP_DPE_BASE + 0x120)
#define DPE_DVE_DBG_INFO_01_REG        (ISP_DPE_BASE + 0x124)
#define DPE_DVE_DBG_INFO_02_REG        (ISP_DPE_BASE + 0x128)
#define DPE_DVE_DBG_INFO_03_REG        (ISP_DPE_BASE + 0x12C)
#define DPE_DVE_DBG_INFO_04_REG        (ISP_DPE_BASE + 0x130)
#define DPE_DVE_DBG_INFO_05_REG        (ISP_DPE_BASE + 0x134)
#define DPE_DVE_DBG_INFO_06_REG        (ISP_DPE_BASE + 0x138)
#define DPE_DVE_DBG_INFO_07_REG        (ISP_DPE_BASE + 0x13C)
#define DPE_DVE_DBG_INFO_08_REG        (ISP_DPE_BASE + 0x140)
#define DPE_DVE_DBG_INFO_09_REG        (ISP_DPE_BASE + 0x144)

#define DPE_WMFE_START_REG             (ISP_DPE_BASE + 0x220)
#define DPE_WMFE_INT_CTRL_REG          (ISP_DPE_BASE + 0x224)
#define DPE_WMFE_INT_STATUS_REG        (ISP_DPE_BASE + 0x228)

#define DPE_WMFE_CTRL_0_REG            (ISP_DPE_BASE + 0x230)
#define DPE_WMFE_SIZE_0_REG            (ISP_DPE_BASE + 0x234)
#define DPE_WMFE_IMGI_BASE_ADDR_0_REG  (ISP_DPE_BASE + 0x238)
#define DPE_WMFE_IMGI_STRIDE_0_REG     (ISP_DPE_BASE + 0x23C)
#define DPE_WMFE_DPI_BASE_ADDR_0_REG   (ISP_DPE_BASE + 0x240)
#define DPE_WMFE_DPI_STRIDE_0_REG      (ISP_DPE_BASE + 0x244)
#define DPE_WMFE_TBLI_BASE_ADDR_0_REG  (ISP_DPE_BASE + 0x248)
#define DPE_WMFE_TBLI_STRIDE_0_REG     (ISP_DPE_BASE + 0x24C)
#define DPE_WMFE_MASKI_BASE_ADDR_0_REG (ISP_DPE_BASE + 0x250)
#define DPE_WMFE_MASKI_STRIDE_0_REG    (ISP_DPE_BASE + 0x254)
#define DPE_WMFE_DPO_BASE_ADDR_0_REG   (ISP_DPE_BASE + 0x258)
#define DPE_WMFE_DPO_STRIDE_0_REG      (ISP_DPE_BASE + 0x25C)

#define DPE_WMFE_CTRL_1_REG            (ISP_DPE_BASE + 0x270)
#define DPE_WMFE_SIZE_1_REG            (ISP_DPE_BASE + 0x274)
#define DPE_WMFE_IMGI_BASE_ADDR_1_REG  (ISP_DPE_BASE + 0x278)
#define DPE_WMFE_IMGI_STRIDE_1_REG     (ISP_DPE_BASE + 0x27C)
#define DPE_WMFE_DPI_BASE_ADDR_1_REG   (ISP_DPE_BASE + 0x280)
#define DPE_WMFE_DPI_STRIDE_1_REG      (ISP_DPE_BASE + 0x284)
#define DPE_WMFE_TBLI_BASE_ADDR_1_REG  (ISP_DPE_BASE + 0x288)
#define DPE_WMFE_TBLI_STRIDE_1_REG     (ISP_DPE_BASE + 0x28C)
#define DPE_WMFE_MASKI_BASE_ADDR_1_REG (ISP_DPE_BASE + 0x290)
#define DPE_WMFE_MASKI_STRIDE_1_REG    (ISP_DPE_BASE + 0x294)
#define DPE_WMFE_DPO_BASE_ADDR_1_REG   (ISP_DPE_BASE + 0x298)
#define DPE_WMFE_DPO_STRIDE_1_REG      (ISP_DPE_BASE + 0x29C)


#define DPE_WMFE_CTRL_2_REG            (ISP_DPE_BASE + 0x2B0)
#define DPE_WMFE_SIZE_2_REG            (ISP_DPE_BASE + 0x2B4)
#define DPE_WMFE_IMGI_BASE_ADDR_2_REG  (ISP_DPE_BASE + 0x2B8)
#define DPE_WMFE_IMGI_STRIDE_2_REG     (ISP_DPE_BASE + 0x2BC)
#define DPE_WMFE_DPI_BASE_ADDR_2_REG   (ISP_DPE_BASE + 0x2C0)
#define DPE_WMFE_DPI_STRIDE_2_REG      (ISP_DPE_BASE + 0x2C4)
#define DPE_WMFE_TBLI_BASE_ADDR_2_REG  (ISP_DPE_BASE + 0x2C8)
#define DPE_WMFE_TBLI_STRIDE_2_REG     (ISP_DPE_BASE + 0x2CC)
#define DPE_WMFE_MASKI_BASE_ADDR_2_REG (ISP_DPE_BASE + 0x2D0)
#define DPE_WMFE_MASKI_STRIDE_2_REG    (ISP_DPE_BASE + 0x2D4)
#define DPE_WMFE_DPO_BASE_ADDR_2_REG   (ISP_DPE_BASE + 0x2D8)
#define DPE_WMFE_DPO_STRIDE_2_REG      (ISP_DPE_BASE + 0x2DC)

#define DPE_WMFE_CTRL_3_REG            (ISP_DPE_BASE + 0x2F0)
#define DPE_WMFE_SIZE_3_REG            (ISP_DPE_BASE + 0x2F4)
#define DPE_WMFE_IMGI_BASE_ADDR_3_REG  (ISP_DPE_BASE + 0x2F8)
#define DPE_WMFE_IMGI_STRIDE_3_REG     (ISP_DPE_BASE + 0x2FC)
#define DPE_WMFE_DPI_BASE_ADDR_3_REG   (ISP_DPE_BASE + 0x300)
#define DPE_WMFE_DPI_STRIDE_3_REG      (ISP_DPE_BASE + 0x304)
#define DPE_WMFE_TBLI_BASE_ADDR_3_REG  (ISP_DPE_BASE + 0x308)
#define DPE_WMFE_TBLI_STRIDE_3_REG     (ISP_DPE_BASE + 0x30C)
#define DPE_WMFE_MASKI_BASE_ADDR_3_REG (ISP_DPE_BASE + 0x310)
#define DPE_WMFE_MASKI_STRIDE_3_REG    (ISP_DPE_BASE + 0x314)
#define DPE_WMFE_DPO_BASE_ADDR_3_REG   (ISP_DPE_BASE + 0x318)
#define DPE_WMFE_DPO_STRIDE_3_REG      (ISP_DPE_BASE + 0x31C)

#define DPE_WMFE_CTRL_4_REG            (ISP_DPE_BASE + 0x330)
#define DPE_WMFE_SIZE_4_REG            (ISP_DPE_BASE + 0x334)
#define DPE_WMFE_IMGI_BASE_ADDR_4_REG  (ISP_DPE_BASE + 0x338)
#define DPE_WMFE_IMGI_STRIDE_4_REG     (ISP_DPE_BASE + 0x33C)
#define DPE_WMFE_DPI_BASE_ADDR_4_REG   (ISP_DPE_BASE + 0x340)
#define DPE_WMFE_DPI_STRIDE_4_REG      (ISP_DPE_BASE + 0x344)
#define DPE_WMFE_TBLI_BASE_ADDR_4_REG  (ISP_DPE_BASE + 0x348)
#define DPE_WMFE_TBLI_STRIDE_4_REG     (ISP_DPE_BASE + 0x34C)
#define DPE_WMFE_MASKI_BASE_ADDR_4_REG (ISP_DPE_BASE + 0x350)
#define DPE_WMFE_MASKI_STRIDE_4_REG    (ISP_DPE_BASE + 0x354)
#define DPE_WMFE_DPO_BASE_ADDR_4_REG   (ISP_DPE_BASE + 0x358)
#define DPE_WMFE_DPO_STRIDE_4_REG      (ISP_DPE_BASE + 0x35C)


#define DPE_WMFE_DBG_INFO_00_REG       (ISP_DPE_BASE + 0x400)
#define DPE_WMFE_DBG_INFO_01_REG       (ISP_DPE_BASE + 0x404)
#define DPE_WMFE_DBG_INFO_02_REG       (ISP_DPE_BASE + 0x408)
#define DPE_WMFE_DBG_INFO_03_REG       (ISP_DPE_BASE + 0x40C)
#define DPE_WMFE_DBG_INFO_04_REG       (ISP_DPE_BASE + 0x410)
#define DPE_WMFE_DBG_INFO_05_REG       (ISP_DPE_BASE + 0x414)
#define DPE_WMFE_DBG_INFO_06_REG       (ISP_DPE_BASE + 0x418)
#define DPE_WMFE_DBG_INFO_07_REG       (ISP_DPE_BASE + 0x41C)
#define DPE_WMFE_DBG_INFO_08_REG       (ISP_DPE_BASE + 0x420)
#define DPE_WMFE_DBG_INFO_09_REG       (ISP_DPE_BASE + 0x424)

#define DPE_DMA_DBG_REG                (ISP_DPE_BASE + 0x7F4)
#define DPE_DMA_REQ_STATUS_REG         (ISP_DPE_BASE + 0x7F8)
#define DPE_DMA_RDY_STATUS_REG         (ISP_DPE_BASE + 0x7FC)

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 DPE_MsToJiffies(MUINT32 Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 DPE_UsToJiffies(MUINT32 Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 DPE_GetIRQState(MUINT32 type, MUINT32 userNumber, MUINT32 stus,
				      DPE_PROCESS_ID_ENUM whichReq, int ProcessID)
{
	MUINT32 ret = 0;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */

	/*  */
	spin_lock_irqsave(&(DPEInfo.SpinLockIrq[type]), flags);
#ifdef DPE_USE_GCE

#ifdef DPE_MULTIPROCESS_TIMEING_ISSUE
	if (stus & DPE_DVE_INT_ST) {
		ret = ((DPEInfo.IrqInfo.DveIrqCnt > 0)
		       && (DPEInfo.ProcessID[DPEInfo.ReadReqIdx] == ProcessID));
	} else if (stus & DPE_WMFE_INT_ST) {
		ret = ((DPEInfo.IrqInfo.WmfeIrqCnt > 0)
		       && (DPEInfo.ProcessID[DPEInfo.ReadReqIdx] == ProcessID));
	} else {
		LOG_ERR(" WaitIRQ Err,type:%d,urNum:%d,sta:%d,whReq:%d,PID:0x%x, RdReqIdx:%d\n",
		type, userNumber, stus, whichReq, ProcessID, DPEInfo.ReadReqIdx);
	}

#else
	if (stus & DPE_DVE_INT_ST) {
		ret = ((DPEInfo.IrqInfo.DveIrqCnt > 0)
		       && (DPEInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
	} else if (stus & DPE_WMFE_INT_ST) {
		ret = ((DPEInfo.IrqInfo.WmfeIrqCnt > 0)
		       && (DPEInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
	} else {
		LOG_ERR
		    ("WaitIRQ Status Error, type:%d, userNumber:%d, status:%d, whichReq:%d, ProcessID:0x%x\n",
		     type, userNumber, stus, whichReq, ProcessID);
	}
#endif
#else
	ret = ((DPEInfo.IrqInfo.Status[type] & stus)
	       && (DPEInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
#endif
	spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[type]), flags);
	/*  */
	return ret;
}


/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 DPE_JiffiesToMs(MUINT32 Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}


#define RegDump(start, end) {\
	MUINT32 i;\
	for (i = start; i <= end; i += 0x10) {\
		LOG_DBG("[0x%08X %08X],[0x%08X %08X],[0x%08X %08X],[0x%08X %08X]",\
	    (unsigned int)(ISP_DPE_BASE + i), (unsigned int)DPE_RD32(ISP_DPE_BASE + i),\
	    (unsigned int)(ISP_DPE_BASE + i+0x4), (unsigned int)DPE_RD32(ISP_DPE_BASE + i+0x4),\
	    (unsigned int)(ISP_DPE_BASE + i+0x8), (unsigned int)DPE_RD32(ISP_DPE_BASE + i+0x8),\
	    (unsigned int)(ISP_DPE_BASE + i+0xc), (unsigned int)DPE_RD32(ISP_DPE_BASE + i+0xc));\
	} \
}


static MBOOL ConfigDVERequest(MINT32 ReqIdx)
{
#ifdef DPE_USE_GCE
	MUINT32 j;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */


	spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]), flags);
	if (g_DVE_RequestRing.DVEReq_Struct[ReqIdx].RequestState == DPE_REQUEST_STATE_PENDING) {
		g_DVE_RequestRing.DVEReq_Struct[ReqIdx].RequestState = DPE_REQUEST_STATE_RUNNING;
		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
			if (DPE_FRAME_STATUS_ENQUE ==
			    g_DVE_RequestRing.DVEReq_Struct[ReqIdx].DveFrameStatus[j]) {
				g_DVE_RequestRing.DVEReq_Struct[ReqIdx].DveFrameStatus[j] =
				    DPE_FRAME_STATUS_RUNNING;
				spin_unlock_irqrestore(&
						       (DPEInfo.
							SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						       flags);
				ConfigDVEHW(&g_DVE_RequestRing.DVEReq_Struct[ReqIdx].
					    DveFrameConfig[j]);
				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						  flags);
			}
		}
	} else {
		LOG_ERR("ConfigDVERequest state machine error!!, ReqIdx:%d, RequestState:%d\n",
			ReqIdx, g_DVE_RequestRing.DVEReq_Struct[ReqIdx].RequestState);
	}
	spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]), flags);


	return MTRUE;
#else
	LOG_ERR("ConfigDVERequest don't support this mode.!!\n");
	return MFALSE;
#endif
}

static MBOOL UpdateDVE(MUINT32 DpeDveSta0, pid_t *ProcessID)
{
#ifdef DPE_USE_GCE

	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_DVE_RequestRing.HWProcessIdx; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		if (g_DVE_RequestRing.DVEReq_Struct[i].RequestState == DPE_REQUEST_STATE_RUNNING) {
			for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
				if (DPE_FRAME_STATUS_RUNNING ==
				    g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateDVE idx j:%d, DpeDveSta0:0x%x\n", j, DpeDveSta0);
			if (j != _SUPPORT_MAX_DPE_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j] =
				    DPE_FRAME_STATUS_FINISHED;
				g_DVE_RequestRing.DVEReq_Struct[i].DveFrameConfig[j].DPE_DVE_STA_0 =
				    DpeDveSta0;

				if ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ > (next_idx))
					&& (DPE_FRAME_STATUS_EMPTY ==
					    g_DVE_RequestRing.DVEReq_Struct[i].
					    DveFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) = g_DVE_RequestRing.DVEReq_Struct[i].processID;
					g_DVE_RequestRing.DVEReq_Struct[i].RequestState =
					    DPE_REQUEST_STATE_FINISHED;
					g_DVE_RequestRing.HWProcessIdx =
					    (g_DVE_RequestRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish DVE Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_DVE_RequestRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish DVE Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_DVE_RequestRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR,
				       "DVE State Machine is wrong! HWProcessIdx(%d), RequestState(%d)\n",
				       g_DVE_RequestRing.HWProcessIdx,
				       g_DVE_RequestRing.DVEReq_Struct[i].RequestState);
			g_DVE_RequestRing.DVEReq_Struct[i].RequestState =
			    DPE_REQUEST_STATE_FINISHED;
			g_DVE_RequestRing.HWProcessIdx =
			    (g_DVE_RequestRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}
	return bFinishRequest;


#else				/* #ifdef DPE_USE_GCE */
	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_DVE_RequestRing.HWProcessIdx; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		if (g_DVE_RequestRing.DVEReq_Struct[i].RequestState == DPE_REQUEST_STATE_PENDING) {
			for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
				if (DPE_FRAME_STATUS_RUNNING ==
				    g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateDVE idx j:%d, DpeDveSta0:0x%x\n", j, DpeDveSta0);
			if (j != _SUPPORT_MAX_DPE_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j] =
				    DPE_FRAME_STATUS_FINISHED;
				g_DVE_RequestRing.DVEReq_Struct[i].DveFrameConfig[j].DPE_DVE_STA_0 =
				    DpeDveSta0;

				if ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ > (next_idx))
					&& (DPE_FRAME_STATUS_EMPTY ==
					    g_DVE_RequestRing.DVEReq_Struct[i].
					    DveFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) = g_DVE_RequestRing.DVEReq_Struct[i].processID;
					g_DVE_RequestRing.DVEReq_Struct[i].RequestState =
					    DPE_REQUEST_STATE_FINISHED;
					g_DVE_RequestRing.HWProcessIdx =
					    (g_DVE_RequestRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish DVE Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_DVE_RequestRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish DVE Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_DVE_RequestRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR,
				       "DVE State Machine is wrong! HWProcessIdx(%d), RequestState(%d)\n",
				       g_DVE_RequestRing.HWProcessIdx,
				       g_DVE_RequestRing.DVEReq_Struct[i].RequestState);
			g_DVE_RequestRing.DVEReq_Struct[i].RequestState =
			    DPE_REQUEST_STATE_FINISHED;
			g_DVE_RequestRing.HWProcessIdx =
			    (g_DVE_RequestRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}
	return bFinishRequest;
#endif

}


static MBOOL ConfigWMFERequest(MINT32 ReqIdx)
{
#ifdef DPE_USE_GCE
	MUINT32 j;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */


	spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]), flags);
	if (g_WMFE_ReqRing.WMFEReq_Struct[ReqIdx].RequestState == DPE_REQUEST_STATE_PENDING) {
		g_WMFE_ReqRing.WMFEReq_Struct[ReqIdx].RequestState = DPE_REQUEST_STATE_RUNNING;
		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
			if (DPE_FRAME_STATUS_ENQUE ==
			    g_WMFE_ReqRing.WMFEReq_Struct[ReqIdx].WmfeFrameStatus[j]) {
				g_WMFE_ReqRing.WMFEReq_Struct[ReqIdx].WmfeFrameStatus[j] =
				    DPE_FRAME_STATUS_RUNNING;
				spin_unlock_irqrestore(&
						       (DPEInfo.
							SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						       flags);
				ConfigWMFEHW(&g_WMFE_ReqRing.WMFEReq_Struct[ReqIdx].
					     WmfeFrameConfig[j]);
				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						  flags);
			}
		}
	} else {
		LOG_ERR("ConfigWMFERequest state machine error!!, ReqIdx:%d, RequestState:%d\n",
			ReqIdx, g_WMFE_ReqRing.WMFEReq_Struct[ReqIdx].RequestState);
	}
	spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]), flags);


	return MTRUE;
#else
	LOG_ERR("ConfigWMFERequest don't support this mode.!!\n");
	return MFALSE;
#endif
}

static MINT32 ConfigDVEHW(DPE_DVEConfig *pDveConfig)
{
#ifdef DPE_USE_GCE
		struct cmdqRecStruct *handle;
		uint64_t engineFlag = (1L << CMDQ_ENG_DPE);
#endif

	if (DPE_DBG_DBGLOG == (DPE_DBG_DBGLOG & DPEInfo.DebugMask)) {
		LOG_DBG("ConfigDVEHW Start!\n");

		LOG_DBG("DPE_DVE_CTRL_REG:0x%x!\n", pDveConfig->DPE_DVE_CTRL);

		LOG_DBG("DPE_DVE_ORG_L_HORZ_BBOX_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_L_HORZ_BBOX);
		LOG_DBG("DPE_DVE_ORG_L_VERT_BBOX_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_L_VERT_BBOX);
		LOG_DBG("DPE_DVE_ORG_R_HORZ_BBOX_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_R_HORZ_BBOX);
		LOG_DBG("DPE_DVE_ORG_R_VERT_BBOX_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_R_VERT_BBOX);

		LOG_DBG("DPE_DVE_ORG_SIZE_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_SIZE);
		LOG_DBG("DPE_DVE_ORG_SR_0_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_SR_0);
		LOG_DBG("DPE_DVE_ORG_SR_1_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_SR_1);
		LOG_DBG("DPE_DVE_ORG_SV_REG:0x%x!\n", pDveConfig->DPE_DVE_ORG_SV);

		LOG_DBG("DPE_DVE_CAND_NUM_REG:0x%x!\n", pDveConfig->DPE_DVE_CAND_NUM);
		LOG_DBG("DPE_DVE_CAND_SEL_0_REG:0x%x!\n", pDveConfig->DPE_DVE_CAND_SEL_0);
		LOG_DBG("DPE_DVE_CAND_SEL_1_REG:0x%x!\n", pDveConfig->DPE_DVE_CAND_SEL_1);
		LOG_DBG("DPE_DVE_CAND_SEL_2_REG:0x%x!\n", pDveConfig->DPE_DVE_CAND_SEL_2);
		LOG_DBG("DPE_DVE_CAND_TYPE_0_REG:0x%x!\n", pDveConfig->DPE_DVE_CAND_TYPE_0);
		LOG_DBG("DPE_DVE_CAND_TYPE_1_REG:0x%x!\n", pDveConfig->DPE_DVE_CAND_TYPE_1);

		LOG_DBG("DPE_DVE_RAND_LUT_REG:0x%x!\n", pDveConfig->DPE_DVE_RAND_LUT);
		LOG_DBG("DPE_DVE_GMV_REG:0x%x!\n", pDveConfig->DPE_DVE_GMV);
		LOG_DBG("DPE_DVE_DV_INI_REG:0x%x!\n", pDveConfig->DPE_DVE_DV_INI);
		LOG_DBG("DPE_DVE_BLK_VAR_CTRL_REG:0x%x!\n", pDveConfig->DPE_DVE_BLK_VAR_CTRL);
		LOG_DBG("DPE_DVE_SMTH_LUMA_CTRL_REG:0x%x!\n", pDveConfig->DPE_DVE_SMTH_LUMA_CTRL);
		LOG_DBG("DPE_DVE_SMTH_DV_CTRL_REG:0x%x!\n", pDveConfig->DPE_DVE_SMTH_DV_CTRL);

		LOG_DBG("DPE_DVE_ORD_CTRL_REG:0x%x!\n", pDveConfig->DPE_DVE_ORD_CTRL);
		LOG_DBG("DPE_DVE_TYPE_CTRL_0_REG:0x%x!\n", pDveConfig->DPE_DVE_TYPE_CTRL_0);
		LOG_DBG("DPE_DVE_TYPE_CTRL_1_REG:0x%x!\n", pDveConfig->DPE_DVE_TYPE_CTRL_1);

		LOG_DBG("DPE_DVE_IMGI_L_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_IMGI_L_BASE_ADDR);
		LOG_DBG("DPE_DVE_IMGI_R_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_IMGI_R_BASE_ADDR);
		LOG_DBG("DPE_DVE_DVI_L_BASE_ADDR_REG:0x%x!\n", pDveConfig->DPE_DVE_DVI_L_BASE_ADDR);
		LOG_DBG("DPE_DVE_DVI_R_BASE_ADDR_REG:0x%x!\n", pDveConfig->DPE_DVE_DVI_R_BASE_ADDR);
		LOG_DBG("DPE_DVE_MASKI_L_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_MASKI_L_BASE_ADDR);
		LOG_DBG("DPE_DVE_MASKI_R_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_MASKI_R_BASE_ADDR);


		LOG_DBG("DPE_DVE_DVO_L_BASE_ADDR_REG:0x%x!\n", pDveConfig->DPE_DVE_DVO_L_BASE_ADDR);
		LOG_DBG("DPE_DVE_DVO_R_BASE_ADDR_REG:0x%x!\n", pDveConfig->DPE_DVE_DVO_R_BASE_ADDR);
		LOG_DBG("DPE_DVE_CONFO_L_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_CONFO_L_BASE_ADDR);
		LOG_DBG("DPE_DVE_CONFO_R_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_CONFO_R_BASE_ADDR);
		LOG_DBG("DPE_DVE_RESPO_L_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_RESPO_L_BASE_ADDR);
		LOG_DBG("DPE_DVE_RESPO_R_BASE_ADDR_REG:0x%x!\n",
			pDveConfig->DPE_DVE_RESPO_R_BASE_ADDR);

	}
#ifdef DPE_USE_GCE

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigDVEHW");
#endif


	cmdqRecCreate(CMDQ_SCENARIO_KERNEL_CONFIG_GENERAL, &handle);
	/* CMDQ driver dispatches CMDQ HW thread and HW thread's priority according to scenario */

	cmdqRecSetEngine(handle, engineFlag);

	cmdqRecReset(handle);

	/* Use command queue to write register */
	cmdqRecWrite(handle, DPE_INT_CTL_HW, 0x1, CMDQ_REG_MASK);	/* DPE Interrupt read-clear mode */
	cmdqRecWrite(handle, DPE_DVE_INT_CTRL_HW, 0x1, CMDQ_REG_MASK);	/* DVE Interrupt read-clear mode */


	cmdqRecWrite(handle, DPE_DVE_CTRL_HW, pDveConfig->DPE_DVE_CTRL, CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_ORG_L_HORZ_BBOX_HW, (pDveConfig->DPE_DVE_ORG_L_HORZ_BBOX),
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORG_L_VERT_BBOX_HW, (pDveConfig->DPE_DVE_ORG_L_VERT_BBOX),
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORG_R_HORZ_BBOX_HW, (pDveConfig->DPE_DVE_ORG_R_HORZ_BBOX),
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORG_R_VERT_BBOX_HW, (pDveConfig->DPE_DVE_ORG_R_VERT_BBOX),
		     CMDQ_REG_MASK);


	cmdqRecWrite(handle, DPE_DVE_ORG_SIZE_HW, pDveConfig->DPE_DVE_ORG_SIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORG_SR_0_HW, pDveConfig->DPE_DVE_ORG_SR_0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORG_SR_1_HW, pDveConfig->DPE_DVE_ORG_SR_1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORG_SV_HW, pDveConfig->DPE_DVE_ORG_SV, CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_CAND_NUM_HW, pDveConfig->DPE_DVE_CAND_NUM, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CAND_SEL_0_HW, pDveConfig->DPE_DVE_CAND_SEL_0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CAND_SEL_1_HW, pDveConfig->DPE_DVE_CAND_SEL_1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CAND_SEL_2_HW, pDveConfig->DPE_DVE_CAND_SEL_2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CAND_TYPE_0_HW, pDveConfig->DPE_DVE_CAND_TYPE_0,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CAND_TYPE_1_HW, pDveConfig->DPE_DVE_CAND_TYPE_1,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_RAND_LUT_HW, pDveConfig->DPE_DVE_RAND_LUT, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_GMV_HW, pDveConfig->DPE_DVE_GMV, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DV_INI_HW, pDveConfig->DPE_DVE_DV_INI, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_BLK_VAR_CTRL_HW, pDveConfig->DPE_DVE_BLK_VAR_CTRL,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_SMTH_LUMA_CTRL_HW, pDveConfig->DPE_DVE_SMTH_LUMA_CTRL,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_SMTH_DV_CTRL_HW, pDveConfig->DPE_DVE_SMTH_DV_CTRL,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_ORD_CTRL_HW, pDveConfig->DPE_DVE_ORD_CTRL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_TYPE_CTRL_0_HW, pDveConfig->DPE_DVE_TYPE_CTRL_0,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_TYPE_CTRL_1_HW, pDveConfig->DPE_DVE_TYPE_CTRL_1,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_TYPE_CTRL_2_HW, 0x20A0, CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_IMGI_L_BASE_ADDR_HW, pDveConfig->DPE_DVE_IMGI_L_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_IMGI_L_STRIDE_HW, pDveConfig->DPE_DVE_IMGI_L_STRIDE,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_IMGI_R_BASE_ADDR_HW, pDveConfig->DPE_DVE_IMGI_R_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_IMGI_R_STRIDE_HW, pDveConfig->DPE_DVE_IMGI_R_STRIDE,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_DVI_L_BASE_ADDR_HW, pDveConfig->DPE_DVE_DVI_L_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DVI_L_STRIDE_HW, pDveConfig->DPE_DVE_DVI_L_STRIDE,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DVI_R_BASE_ADDR_HW, pDveConfig->DPE_DVE_DVI_R_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DVI_R_STRIDE_HW, pDveConfig->DPE_DVE_DVI_R_STRIDE,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_MASKI_L_BASE_ADDR_HW, pDveConfig->DPE_DVE_MASKI_L_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_MASKI_L_STRIDE_HW, pDveConfig->DPE_DVE_MASKI_L_STRIDE,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_MASKI_R_BASE_ADDR_HW, pDveConfig->DPE_DVE_MASKI_R_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_MASKI_R_STRIDE_HW, pDveConfig->DPE_DVE_MASKI_R_STRIDE,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_DVO_L_BASE_ADDR_HW, pDveConfig->DPE_DVE_DVO_L_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DVO_L_STRIDE_HW, pDveConfig->DPE_DVE_DVO_L_STRIDE,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DVO_R_BASE_ADDR_HW, pDveConfig->DPE_DVE_DVO_R_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_DVO_R_STRIDE_HW, pDveConfig->DPE_DVE_DVO_R_STRIDE,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_CONFO_L_BASE_ADDR_HW, pDveConfig->DPE_DVE_CONFO_L_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CONFO_L_STRIDE_HW, pDveConfig->DPE_DVE_CONFO_L_STRIDE,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CONFO_R_BASE_ADDR_HW, pDveConfig->DPE_DVE_CONFO_R_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_CONFO_R_STRIDE_HW, pDveConfig->DPE_DVE_CONFO_R_STRIDE,
		     CMDQ_REG_MASK);

	cmdqRecWrite(handle, DPE_DVE_RESPO_L_BASE_ADDR_HW, pDveConfig->DPE_DVE_RESPO_L_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_RESPO_L_STRIDE_HW, pDveConfig->DPE_DVE_RESPO_L_STRIDE,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_RESPO_R_BASE_ADDR_HW, pDveConfig->DPE_DVE_RESPO_R_BASE_ADDR,
		     CMDQ_REG_MASK);
	cmdqRecWrite(handle, DPE_DVE_RESPO_R_STRIDE_HW, pDveConfig->DPE_DVE_RESPO_R_STRIDE,
		     CMDQ_REG_MASK);


	cmdqRecWrite(handle, DPE_DVE_START_HW, 0x1, CMDQ_REG_MASK);	/* DPE Interrupt read-clear mode */

	cmdqRecWait(handle, CMDQ_EVENT_DVE_EOF);
	cmdqRecWrite(handle, DPE_DVE_START_HW, 0x0, CMDQ_REG_MASK);	/* DPE Interrupt read-clear mode */

	/* non-blocking API, Please  use cmdqRecFlushAsync() */
	cmdqRecFlushAsync(handle);
	cmdqRecReset(handle);	/* if you want to re-use the handle, please reset the handle */
	cmdqRecDestroy(handle);	/* recycle the memory */

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#else				/* #ifdef DPE_USE_GCE */

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigDVEHW");
#endif


	DPE_WR32(DPE_INT_CTL_REG, 0x1);	/* DPE Interrupt read-clear mode */
	DPE_WR32(DPE_DVE_INT_CTRL_REG, 0x1);	/* DVE Interrupt read-clear mode */


	DPE_WR32(DPE_DVE_CTRL_REG, pDveConfig->DPE_DVE_CTRL);

	DPE_WR32(DPE_DVE_ORG_L_HORZ_BBOX_REG, (pDveConfig->DPE_DVE_ORG_L_HORZ_BBOX));
	DPE_WR32(DPE_DVE_ORG_L_VERT_BBOX_REG, (pDveConfig->DPE_DVE_ORG_L_VERT_BBOX));
	DPE_WR32(DPE_DVE_ORG_R_HORZ_BBOX_REG, (pDveConfig->DPE_DVE_ORG_R_HORZ_BBOX));
	DPE_WR32(DPE_DVE_ORG_R_VERT_BBOX_REG, (pDveConfig->DPE_DVE_ORG_R_VERT_BBOX));


	DPE_WR32(DPE_DVE_ORG_SIZE_REG, pDveConfig->DPE_DVE_ORG_SIZE);
	DPE_WR32(DPE_DVE_ORG_SR_0_REG, pDveConfig->DPE_DVE_ORG_SR_0);
	DPE_WR32(DPE_DVE_ORG_SR_1_REG, pDveConfig->DPE_DVE_ORG_SR_1);
	DPE_WR32(DPE_DVE_ORG_SV_REG, pDveConfig->DPE_DVE_ORG_SV);

	DPE_WR32(DPE_DVE_CAND_NUM_REG, pDveConfig->DPE_DVE_CAND_NUM);
	DPE_WR32(DPE_DVE_CAND_SEL_0_REG, pDveConfig->DPE_DVE_CAND_SEL_0);
	DPE_WR32(DPE_DVE_CAND_SEL_1_REG, pDveConfig->DPE_DVE_CAND_SEL_1);
	DPE_WR32(DPE_DVE_CAND_SEL_2_REG, pDveConfig->DPE_DVE_CAND_SEL_2);
	DPE_WR32(DPE_DVE_CAND_TYPE_0_REG, pDveConfig->DPE_DVE_CAND_TYPE_0);
	DPE_WR32(DPE_DVE_CAND_TYPE_1_REG, pDveConfig->DPE_DVE_CAND_TYPE_1);

	DPE_WR32(DPE_DVE_RAND_LUT_REG, pDveConfig->DPE_DVE_RAND_LUT);
	DPE_WR32(DPE_DVE_GMV_REG, pDveConfig->DPE_DVE_GMV);
	DPE_WR32(DPE_DVE_DV_INI_REG, pDveConfig->DPE_DVE_DV_INI);
	DPE_WR32(DPE_DVE_BLK_VAR_CTRL_REG, pDveConfig->DPE_DVE_BLK_VAR_CTRL);
	DPE_WR32(DPE_DVE_SMTH_LUMA_CTRL_REG, pDveConfig->DPE_DVE_SMTH_LUMA_CTRL);
	DPE_WR32(DPE_DVE_SMTH_DV_CTRL_REG, pDveConfig->DPE_DVE_SMTH_DV_CTRL);
	DPE_WR32(DPE_DVE_ORD_CTRL_REG, pDveConfig->DPE_DVE_ORD_CTRL);
	DPE_WR32(DPE_DVE_TYPE_CTRL_0_REG, pDveConfig->DPE_DVE_TYPE_CTRL_0);
	DPE_WR32(DPE_DVE_TYPE_CTRL_1_REG, pDveConfig->DPE_DVE_TYPE_CTRL_1);


	DPE_WR32(DPE_DVE_IMGI_L_BASE_ADDR_REG, pDveConfig->DPE_DVE_IMGI_L_BASE_ADDR);
	DPE_WR32(DPE_DVE_IMGI_L_STRIDE_REG, pDveConfig->DPE_DVE_IMGI_L_STRIDE);
	DPE_WR32(DPE_DVE_IMGI_R_BASE_ADDR_REG, pDveConfig->DPE_DVE_IMGI_R_BASE_ADDR);
	DPE_WR32(DPE_DVE_IMGI_R_STRIDE_REG, pDveConfig->DPE_DVE_IMGI_R_STRIDE);

	DPE_WR32(DPE_DVE_DVI_L_BASE_ADDR_REG, pDveConfig->DPE_DVE_DVI_L_BASE_ADDR);
	DPE_WR32(DPE_DVE_DVI_L_STRIDE_REG, pDveConfig->DPE_DVE_DVI_L_STRIDE);
	DPE_WR32(DPE_DVE_DVI_R_BASE_ADDR_REG, pDveConfig->DPE_DVE_DVI_R_BASE_ADDR);
	DPE_WR32(DPE_DVE_DVI_R_STRIDE_REG, pDveConfig->DPE_DVE_DVI_R_STRIDE);

	DPE_WR32(DPE_DVE_MASKI_L_BASE_ADDR_REG, pDveConfig->DPE_DVE_MASKI_L_BASE_ADDR);
	DPE_WR32(DPE_DVE_MASKI_L_STRIDE_REG, pDveConfig->DPE_DVE_MASKI_L_STRIDE);
	DPE_WR32(DPE_DVE_MASKI_R_BASE_ADDR_REG, pDveConfig->DPE_DVE_MASKI_R_BASE_ADDR);
	DPE_WR32(DPE_DVE_MASKI_R_STRIDE_REG, pDveConfig->DPE_DVE_MASKI_R_STRIDE);

	DPE_WR32(DPE_DVE_DVO_L_BASE_ADDR_REG, pDveConfig->DPE_DVE_DVO_L_BASE_ADDR);
	DPE_WR32(DPE_DVE_DVO_L_STRIDE_REG, pDveConfig->DPE_DVE_DVO_L_STRIDE);
	DPE_WR32(DPE_DVE_DVO_R_BASE_ADDR_REG, pDveConfig->DPE_DVE_DVO_R_BASE_ADDR);
	DPE_WR32(DPE_DVE_DVO_R_STRIDE_REG, pDveConfig->DPE_DVE_DVO_R_STRIDE);

	DPE_WR32(DPE_DVE_CONFO_L_BASE_ADDR_REG, pDveConfig->DPE_DVE_CONFO_L_BASE_ADDR);
	DPE_WR32(DPE_DVE_CONFO_L_STRIDE_REG, pDveConfig->DPE_DVE_CONFO_L_STRIDE);
	DPE_WR32(DPE_DVE_CONFO_R_BASE_ADDR_REG, pDveConfig->DPE_DVE_CONFO_R_BASE_ADDR);
	DPE_WR32(DPE_DVE_CONFO_R_STRIDE_REG, pDveConfig->DPE_DVE_CONFO_R_STRIDE);

	DPE_WR32(DPE_DVE_RESPO_L_BASE_ADDR_REG, pDveConfig->DPE_DVE_RESPO_L_BASE_ADDR);
	DPE_WR32(DPE_DVE_RESPO_L_STRIDE_REG, pDveConfig->DPE_DVE_RESPO_L_STRIDE);
	DPE_WR32(DPE_DVE_RESPO_R_BASE_ADDR_REG, pDveConfig->DPE_DVE_RESPO_R_BASE_ADDR);
	DPE_WR32(DPE_DVE_RESPO_R_STRIDE_REG, pDveConfig->DPE_DVE_RESPO_R_STRIDE);

	DPE_WR32(DPE_DVE_START_REG, 0x1);	/* DPE Interrupt read-clear mode */

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#endif				/* #ifdef DPE_USE_GCE */
	return 0;

}

static MBOOL UpdateWMFE(pid_t *ProcessID)
{
#ifdef DPE_USE_GCE
	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_WMFE_ReqRing.HWProcessIdx; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		if (g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState == DPE_REQUEST_STATE_RUNNING) {
			for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
				if (DPE_FRAME_STATUS_RUNNING ==
				    g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateWMFE idx j:%d\n", j);
			if (j != _SUPPORT_MAX_DPE_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j] =
				    DPE_FRAME_STATUS_FINISHED;
				if ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ > (next_idx))
					&& (DPE_FRAME_STATUS_EMPTY ==
					    g_WMFE_ReqRing.WMFEReq_Struct[i].
					    WmfeFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) =
					    g_WMFE_ReqRing.WMFEReq_Struct[i].processID;
					g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState =
					    DPE_REQUEST_STATE_FINISHED;
					g_WMFE_ReqRing.HWProcessIdx =
					    (g_WMFE_ReqRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish WMFE Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_WMFE_ReqRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish WMFE Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_WMFE_ReqRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR,
				       "WMFE State Machine is wrong! HWProcessIdx(%d), RequestState(%d)\n",
				       g_WMFE_ReqRing.HWProcessIdx,
				       g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState);
			g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState =
			    DPE_REQUEST_STATE_FINISHED;
			g_WMFE_ReqRing.HWProcessIdx =
			    (g_WMFE_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}

	return bFinishRequest;


#else				/* #ifdef DPE_USE_GCE */
	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_WMFE_ReqRing.HWProcessIdx; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		if (g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState == DPE_REQUEST_STATE_PENDING) {
			for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
				if (DPE_FRAME_STATUS_RUNNING ==
				    g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateWMFE idx j:%d\n", j);
			if (j != _SUPPORT_MAX_DPE_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j] =
				    DPE_FRAME_STATUS_FINISHED;
				if ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_DPE_FRAME_REQUEST_ > (next_idx))
					&& (DPE_FRAME_STATUS_EMPTY ==
					    g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) =
					    g_WMFE_ReqRing.WMFEReq_Struct[i].processID;
					g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState =
					    DPE_REQUEST_STATE_FINISHED;
					g_WMFE_ReqRing.HWProcessIdx =
					    (g_WMFE_ReqRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish WMFE Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_WMFE_ReqRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish WMFE Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_WMFE_ReqRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR,
				       "WMFE State Machine is wrong! HWProcessIdx(%d), RequestState(%d)\n",
				       g_WMFE_ReqRing.HWProcessIdx,
				       g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState);
			g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState =
			    DPE_REQUEST_STATE_FINISHED;
			g_WMFE_ReqRing.HWProcessIdx =
			    (g_WMFE_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}

	return bFinishRequest;

#endif				/* #ifdef DPE_USE_GCE */


}


static MINT32 ConfigWMFEHW(DPE_WMFEConfig *pWmfeCfg)
{
#ifdef DPE_USE_GCE
		struct cmdqRecStruct *handle;
		uint64_t engineFlag = (1L << CMDQ_ENG_DPE);
#endif
	unsigned int i = 0;

	if (DPE_DBG_DBGLOG == (DPE_DBG_DBGLOG & DPEInfo.DebugMask)) {

		LOG_DBG("ConfigWMFEHW Start!\n");
		for (i = 0; i < pWmfeCfg->WmfeCtrlSize; i++) {
			LOG_DBG("WMFE_CTRL_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_CTRL);
			LOG_DBG("WMFE_SIZE_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_SIZE);
			LOG_DBG("WMFE_IMGI_BASE_ADDR_%d_REG:0x%x!\n",
				i, pWmfeCfg->WmfeCtrl[i].WMFE_IMGI_BASE_ADDR);
			LOG_DBG("WMFE_IMGI_STRIDE_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_IMGI_STRIDE);
			LOG_DBG("WMFE_DPI_BASE_ADDR_%d_REG:0x%x!\n",
				i, pWmfeCfg->WmfeCtrl[i].WMFE_DPI_BASE_ADDR);
			LOG_DBG("WMFE_DPI_STRIDE_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_DPI_STRIDE);
			LOG_DBG("WMFE_TBLI_BASE_ADDR_%d_REG:0x%x!\n",
				i, pWmfeCfg->WmfeCtrl[i].WMFE_TBLI_BASE_ADDR);
			LOG_DBG("WMFE_TBLI_STRIDE_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_TBLI_STRIDE);
			LOG_DBG("WMFE_MASKI_BASE_ADDR_%d_REG:0x%x!\n",
				i, pWmfeCfg->WmfeCtrl[i].WMFE_MASKI_BASE_ADDR);
			LOG_DBG("WMFE_MASKI_STRIDE_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_MASKI_STRIDE);
			LOG_DBG("WMFE_DPO_BASE_ADDR_%d_REG:0x%x!\n",
				i, pWmfeCfg->WmfeCtrl[i].WMFE_DPO_BASE_ADDR);
			LOG_DBG("WMFE_DPO_STRIDE_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_DPO_STRIDE);
		}
	}
#ifdef DPE_USE_GCE

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigWMFEHW");
#endif

	cmdqRecCreate(CMDQ_SCENARIO_KERNEL_CONFIG_GENERAL, &handle);
	/* CMDQ driver dispatches CMDQ HW thread and HW thread's priority according to scenario */

	cmdqRecSetEngine(handle, engineFlag);

	cmdqRecReset(handle);

	/* Use command queue to write register */
	cmdqRecWrite(handle, DPE_INT_CTL_HW, 0x1, CMDQ_REG_MASK);	/* DPE Interrupt read-clear mode */
	cmdqRecWrite(handle, DPE_WMFE_INT_CTRL_HW, 0x1, CMDQ_REG_MASK);	/* WMFE Interrupt read-clear mode */

	for (i = 0; i < pWmfeCfg->WmfeCtrlSize; i++) {
		LOG_DBG("DPE_WMFE_CTRL_%d_REG:0x%x!\n", i, pWmfeCfg->WmfeCtrl[i].WMFE_CTRL);

		if (WMFE_ENABLE == (pWmfeCfg->WmfeCtrl[i].WMFE_CTRL & WMFE_ENABLE)) {
			cmdqRecWrite(handle, DPE_WMFE_CTRL_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_CTRL, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_SIZE_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_SIZE, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_IMGI_BASE_ADDR_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_IMGI_BASE_ADDR, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_IMGI_STRIDE_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_IMGI_STRIDE, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_DPI_BASE_ADDR_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_DPI_BASE_ADDR, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_DPI_STRIDE_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_DPI_STRIDE, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_TBLI_BASE_ADDR_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_TBLI_BASE_ADDR, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_TBLI_STRIDE_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_TBLI_STRIDE, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_MASKI_BASE_ADDR_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_MASKI_BASE_ADDR, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_MASKI_STRIDE_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_MASKI_STRIDE, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_DPO_BASE_ADDR_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_DPO_BASE_ADDR, CMDQ_REG_MASK);
			cmdqRecWrite(handle, DPE_WMFE_DPO_STRIDE_0_HW + (i*0x40),
				pWmfeCfg->WmfeCtrl[i].WMFE_DPO_STRIDE, CMDQ_REG_MASK);
		}
	}

	cmdqRecWrite(handle, DPE_WMFE_START_HW, 0x1, CMDQ_REG_MASK);	/* DPE Interrupt read-clear mode */

	cmdqRecWait(handle, CMDQ_EVENT_WMF_EOF);
	cmdqRecWrite(handle, DPE_WMFE_START_HW, 0x0, CMDQ_REG_MASK);	/* DPE Interrupt read-clear mode */

	/* non-blocking API, Please  use cmdqRecFlushAsync() */
	cmdqRecFlushAsync(handle);
	cmdqRecReset(handle);	/* if you want to re-use the handle, please reset the handle */
	cmdqRecDestroy(handle);	/* recycle the memory */

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#else

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigWMFEHW");
#endif

	DPE_WR32(DPE_INT_CTL_REG, 0x1);	/* DPE Interrupt read-clear mode */
	DPE_WR32(DPE_WMFE_INT_CTRL_REG, 0x1);	/* WMFE Interrupt read-clear mode */

	if (WMFE_ENABLE == (pWmfeConfig->DPE_WMFE_CTRL_0 & WMFE_ENABLE)) {
		DPE_WR32(DPE_WMFE_CTRL_0_REG, pWmfeConfig->WMFE_CTRL_0);
		DPE_WR32(DPE_WMFE_SIZE_0_REG, pWmfeConfig->WMFE_SIZE_0);
		DPE_WR32(DPE_WMFE_IMGI_BASE_ADDR_0_REG, pWmfeConfig->WMFE_IMGI_BASE_ADDR_0);
		DPE_WR32(DPE_WMFE_IMGI_STRIDE_0_REG, pWmfeConfig->WMFE_IMGI_STRIDE_0);
		DPE_WR32(DPE_WMFE_DPI_BASE_ADDR_0_REG, pWmfeConfig->WMFE_DPI_BASE_ADDR_0);
		DPE_WR32(DPE_WMFE_DPI_STRIDE_0_REG, pWmfeConfig->WMFE_DPI_STRIDE_0);
		DPE_WR32(DPE_WMFE_TBLI_BASE_ADDR_0_REG, pWmfeConfig->WMFE_TBLI_BASE_ADDR_0);
		DPE_WR32(DPE_WMFE_TBLI_STRIDE_0_REG, pWmfeConfig->WMFE_TBLI_STRIDE_0);
		DPE_WR32(DPE_WMFE_DPO_BASE_ADDR_0_REG, pWmfeConfig->WMFE_DPO_BASE_ADDR_0);
		DPE_WR32(DPE_WMFE_DPO_STRIDE_0_REG, pWmfeConfig->WMFE_DPO_STRIDE_0);
	}

	if (WMFE_ENABLE == (pWmfeConfig->DPE_WMFE_CTRL_1 & WMFE_ENABLE)) {
		DPE_WR32(DPE_WMFE_CTRL_1_REG, pWmfeConfig->WMFE_CTRL_1);
		DPE_WR32(DPE_WMFE_SIZE_1_REG, pWmfeConfig->WMFE_SIZE_1);
		DPE_WR32(DPE_WMFE_IMGI_BASE_ADDR_1_REG, pWmfeConfig->WMFE_IMGI_BASE_ADDR_1);
		DPE_WR32(DPE_WMFE_IMGI_STRIDE_1_REG, pWmfeConfig->WMFE_IMGI_STRIDE_1);
		DPE_WR32(DPE_WMFE_DPI_BASE_ADDR_1_REG, pWmfeConfig->WMFE_DPI_BASE_ADDR_1);
		DPE_WR32(DPE_WMFE_DPI_STRIDE_1_REG, pWmfeConfig->WMFE_DPI_STRIDE_1);
		DPE_WR32(DPE_WMFE_TBLI_BASE_ADDR_1_REG, pWmfeConfig->WMFE_TBLI_BASE_ADDR_1);
		DPE_WR32(DPE_WMFE_TBLI_STRIDE_1_REG, pWmfeConfig->WMFE_TBLI_STRIDE_1);
		DPE_WR32(DPE_WMFE_DPO_BASE_ADDR_1_REG, pWmfeConfig->WMFE_DPO_BASE_ADDR_1);
		DPE_WR32(DPE_WMFE_DPO_STRIDE_1_REG, pWmfeConfig->WMFE_DPO_STRIDE_1);
	}

	if (WMFE_ENABLE == (pWmfeConfig->DPE_WMFE_CTRL_2 & WMFE_ENABLE)) {
		DPE_WR32(DPE_WMFE_CTRL_2_REG, pWmfeConfig->WMFE_CTRL_2);
		DPE_WR32(DPE_WMFE_SIZE_2_REG, pWmfeConfig->WMFE_SIZE_2);
		DPE_WR32(DPE_WMFE_IMGI_BASE_ADDR_2_REG, pWmfeConfig->WMFE_IMGI_BASE_ADDR_2);
		DPE_WR32(DPE_WMFE_IMGI_STRIDE_2_REG, pWmfeConfig->WMFE_IMGI_STRIDE_2);
		DPE_WR32(DPE_WMFE_DPI_BASE_ADDR_2_REG, pWmfeConfig->WMFE_DPI_BASE_ADDR_2);
		DPE_WR32(DPE_WMFE_DPI_STRIDE_2_REG, pWmfeConfig->WMFE_DPI_STRIDE_2);
		DPE_WR32(DPE_WMFE_TBLI_BASE_ADDR_2_REG, pWmfeConfig->WMFE_TBLI_BASE_ADDR_2);
		DPE_WR32(DPE_WMFE_TBLI_STRIDE_2_REG, pWmfeConfig->WMFE_TBLI_STRIDE_2);
		DPE_WR32(DPE_WMFE_DPO_BASE_ADDR_2_REG, pWmfeConfig->WMFE_DPO_BASE_ADDR_2);
		DPE_WR32(DPE_WMFE_DPO_STRIDE_2_REG, pWmfeConfig->WMFE_DPO_STRIDE_2);
	}

	DPE_WR32(DPE_WMFE_START_REG, 0x1);	/* DPE Interrupt read-clear mode */

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#endif
	return 0;
}

#define DVE_IS_BUSY     0x1
#define WMFE_IS_BUSY    0x2

#ifndef DPE_USE_GCE

static MBOOL Check_DVE_Is_Busy(void)
{
	MUINT32 Ctrl_Fsm;
	MUINT32 Dve_Sart;

	Ctrl_Fsm = DPE_RD32(DPE_DBG_INFO_0_REG);
	Dve_Sart = DPE_RD32(DPE_DVE_START_REG);
	if ((DVE_IS_BUSY == (Ctrl_Fsm & DVE_IS_BUSY)) || (DVE_START == (Dve_Sart & DVE_START)))
		return MTRUE;

	return MFALSE;
}


static MBOOL Check_WMFE_Is_Busy(void)
{
	MUINT32 Ctrl_Fsm;
	MUINT32 Wmfe_Sart;

	Ctrl_Fsm = DPE_RD32(DPE_DBG_INFO_0_REG);
	Wmfe_Sart = DPE_RD32(DPE_DVE_START_REG);
	if ((WMFE_IS_BUSY == (Ctrl_Fsm & WMFE_IS_BUSY)) || (WMFE_START == (Wmfe_Sart & WMFE_START)))
		return MTRUE;

	return MFALSE;
}
#endif


/*
 *
 */
static MINT32 DPE_DumpReg(void)
{
	MINT32 Ret = 0;
	MUINT32 i, j;
	/*  */
	LOG_INF("- E.");
	/*  */
	LOG_INF("DVE Config Info\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CTRL_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_START_HW),
		(unsigned int)DPE_RD32(DPE_DVE_START_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_L_HORZ_BBOX_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_L_HORZ_BBOX_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_L_VERT_BBOX_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_L_VERT_BBOX_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_R_HORZ_BBOX_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_R_HORZ_BBOX_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_R_VERT_BBOX_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_R_VERT_BBOX_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_SIZE_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_SIZE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_SR_0_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_SR_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORG_SR_1_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_SR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)((DPE_DVE_ORG_SV_HW)),
		(unsigned int)DPE_RD32(DPE_DVE_ORG_SV_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CAND_NUM_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CAND_NUM_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CAND_SEL_0_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CAND_SEL_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CAND_SEL_1_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CAND_SEL_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CAND_SEL_2_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CAND_SEL_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CAND_TYPE_0_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CAND_TYPE_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_CAND_TYPE_1_HW),
		(unsigned int)DPE_RD32(DPE_DVE_CAND_TYPE_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_RAND_LUT_HW),
		(unsigned int)DPE_RD32(DPE_DVE_RAND_LUT_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_GMV_HW),
		(unsigned int)DPE_RD32(DPE_DVE_GMV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DV_INI_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DV_INI_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_BLK_VAR_CTRL_HW),
		(unsigned int)DPE_RD32(DPE_DVE_BLK_VAR_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_SMTH_LUMA_CTRL_HW),
		(unsigned int)DPE_RD32(DPE_DVE_SMTH_LUMA_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_SMTH_DV_CTRL_HW),
		(unsigned int)DPE_RD32(DPE_DVE_SMTH_DV_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_ORD_CTRL_HW),
		(unsigned int)DPE_RD32(DPE_DVE_ORD_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_TYPE_CTRL_0_HW),
		(unsigned int)DPE_RD32(DPE_DVE_TYPE_CTRL_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_TYPE_CTRL_1_HW),
		(unsigned int)DPE_RD32(DPE_DVE_TYPE_CTRL_1_REG));

	LOG_INF("DVE Debug Info\n");

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_00_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_01_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_02_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_03_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_04_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_05_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_06_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_07_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_08_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DVE_DBG_INFO_09_HW),
		(unsigned int)DPE_RD32(DPE_DVE_DBG_INFO_09_REG));


	LOG_INF("WMFE Config Info\n");
	/* WMFE Config0 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_START_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_START_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_CTRL_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_CTRL_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_SIZE_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_SIZE_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_BASE_ADDR_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_BASE_ADDR_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_STRIDE_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_STRIDE_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_BASE_ADDR_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_BASE_ADDR_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_STRIDE_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_STRIDE_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_BASE_ADDR_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_BASE_ADDR_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_STRIDE_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_STRIDE_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_BASE_ADDR_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_BASE_ADDR_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_STRIDE_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_STRIDE_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_BASE_ADDR_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_BASE_ADDR_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_STRIDE_0_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_STRIDE_0_REG));


	/* WMFE Config1 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_CTRL_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_CTRL_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_SIZE_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_SIZE_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_BASE_ADDR_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_BASE_ADDR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_STRIDE_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_STRIDE_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_BASE_ADDR_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_BASE_ADDR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_STRIDE_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_STRIDE_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_BASE_ADDR_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_BASE_ADDR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_STRIDE_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_STRIDE_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_BASE_ADDR_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_BASE_ADDR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_STRIDE_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_STRIDE_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_BASE_ADDR_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_BASE_ADDR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_STRIDE_1_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_STRIDE_1_REG));

	/* WMFE Config2 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_CTRL_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_CTRL_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_SIZE_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_SIZE_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_BASE_ADDR_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_BASE_ADDR_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_STRIDE_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_STRIDE_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_BASE_ADDR_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_BASE_ADDR_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_STRIDE_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_STRIDE_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_BASE_ADDR_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_BASE_ADDR_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_STRIDE_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_STRIDE_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_BASE_ADDR_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_BASE_ADDR_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_STRIDE_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_STRIDE_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_BASE_ADDR_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_BASE_ADDR_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_STRIDE_2_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_STRIDE_2_REG));

	/* WMFE Config3 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_CTRL_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_CTRL_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_SIZE_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_SIZE_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_BASE_ADDR_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_BASE_ADDR_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_STRIDE_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_STRIDE_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_BASE_ADDR_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_BASE_ADDR_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_STRIDE_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_STRIDE_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_BASE_ADDR_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_BASE_ADDR_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_STRIDE_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_STRIDE_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_BASE_ADDR_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_BASE_ADDR_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_STRIDE_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_STRIDE_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_BASE_ADDR_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_BASE_ADDR_3_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_STRIDE_3_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_STRIDE_3_REG));

	/* WMFE Config4 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_CTRL_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_CTRL_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_SIZE_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_SIZE_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_BASE_ADDR_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_BASE_ADDR_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_IMGI_STRIDE_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_IMGI_STRIDE_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_BASE_ADDR_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_BASE_ADDR_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPI_STRIDE_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPI_STRIDE_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_BASE_ADDR_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_BASE_ADDR_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_TBLI_STRIDE_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_TBLI_STRIDE_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_BASE_ADDR_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_BASE_ADDR_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_MASKI_STRIDE_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_MASKI_STRIDE_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_BASE_ADDR_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_BASE_ADDR_4_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DPO_STRIDE_4_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DPO_STRIDE_4_REG));


	LOG_INF("WMFE Debug Info\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_00_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_01_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_02_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_03_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_04_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_05_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_06_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_06_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_07_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_07_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_08_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_08_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_WMFE_DBG_INFO_09_HW),
		(unsigned int)DPE_RD32(DPE_WMFE_DBG_INFO_09_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DMA_DBG_HW),
		(unsigned int)DPE_RD32(DPE_DMA_DBG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DMA_REQ_STATUS_HW),
		(unsigned int)DPE_RD32(DPE_DMA_REQ_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DPE_DMA_RDY_STATUS_HW),
		(unsigned int)DPE_RD32(DPE_DMA_RDY_STATUS_REG));


	LOG_INF("DVE:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n", g_DVE_RequestRing.HWProcessIdx,
		g_DVE_RequestRing.WriteIdx, g_DVE_RequestRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		LOG_INF
		    ("DVE:ReqSte:%d, PID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, FrameRDIdx:%d\n",
		     g_DVE_RequestRing.DVEReq_Struct[i].RequestState,
		     g_DVE_RequestRing.DVEReq_Struct[i].processID,
		     g_DVE_RequestRing.DVEReq_Struct[i].callerID,
		     g_DVE_RequestRing.DVEReq_Struct[i].enqueReqNum,
		     g_DVE_RequestRing.DVEReq_Struct[i].FrameWRIdx,
		     g_DVE_RequestRing.DVEReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_;) {
			LOG_INF
			    ("DVE:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
			     j, g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j]
			     , j + 1, g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j + 1],
			     j + 2, g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j + 2]
			     , j + 3, g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j + 3]);
			j = j + 4;
		}
	}

	LOG_INF("WMFE:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n", g_WMFE_ReqRing.HWProcessIdx,
		g_WMFE_ReqRing.WriteIdx, g_WMFE_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		LOG_INF
		    ("WMFE:ReqSte:%d, PID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, FrameRDIdx:%d\n",
		     g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState,
		     g_WMFE_ReqRing.WMFEReq_Struct[i].processID,
		     g_WMFE_ReqRing.WMFEReq_Struct[i].callerID,
		     g_WMFE_ReqRing.WMFEReq_Struct[i].enqueReqNum,
		     g_WMFE_ReqRing.WMFEReq_Struct[i].FrameWRIdx,
		     g_WMFE_ReqRing.WMFEReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_;) {
			LOG_INF
			    ("WMFE:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
			     j, g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j]
			     , j + 1, g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j + 1],
			     j + 2, g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j + 2]
			     , j + 3, g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j + 3]);
			j = j + 4;
		}

	}



	LOG_INF("- X.");
	/*  */
	return Ret;
}
#ifndef __DPE_EP_NO_CLKMGR__
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
static inline void DPE_Prepare_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_DIS-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> DPE clk */
	smi_clk_prepare(SMI_LARB_IMGSYS1, "camera-dpe", 1);

	ret = clk_prepare(dpe_clk.CG_IMGSYS_DPE);
	if (ret)
		LOG_ERR("cannot prepare CG_IMGSYS_DPE clock\n");

}

static inline void DPE_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_DIS-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> DPE  clk */
	smi_clk_enable(SMI_LARB_IMGSYS1, "camera-dpe", 1);

	ret = clk_enable(dpe_clk.CG_IMGSYS_DPE);
	if (ret)
		LOG_ERR("cannot enable CG_IMGSYS_DPE clock\n");

}

static inline void DPE_Prepare_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_DIS-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> DPE clk */
	smi_bus_enable(SMI_LARB_IMGSYS1, "camera-dpe");

	ret = clk_prepare_enable(dpe_clk.CG_IMGSYS_DPE);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_DPE clock\n");

}

static inline void DPE_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: DPE clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_DIS */
	clk_unprepare(dpe_clk.CG_IMGSYS_DPE);
	smi_clk_unprepare(SMI_LARB_IMGSYS1, "camera-dpe", 1);

}

static inline void DPE_Disable_ccf_clock(void)
{
	/* must keep this clk close order: DPE clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_DIS */
	clk_disable(dpe_clk.CG_IMGSYS_DPE);
	smi_clk_disable(SMI_LARB_IMGSYS1, "camera-dpe", 1);
}

static inline void DPE_Disable_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: DPE clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_DIS */

	clk_disable_unprepare(dpe_clk.CG_IMGSYS_DPE);
	smi_bus_disable(SMI_LARB_IMGSYS1, "camera-dpe");
}
#endif
#endif
/*******************************************************************************
*
********************************************************************************/
static void DPE_EnableClock(MBOOL En)
{
	if (En) {		/* Enable clock. */
		/* LOG_DBG("Dpe clock enbled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		switch (g_u4EnableClockCount) {
		case 0:
#ifndef __DPE_EP_NO_CLKMGR__
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
			    DPE_Prepare_Enable_ccf_clock();
#else
			enable_clock(MT_CG_DDPE0_SMI_COMMON, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
			enable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
			enable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_SV, "CAMERA");
			/* enable_clock(MT_CG_IMAGE_FD, "CAMERA"); */
			enable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
#endif				/* #if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
#endif
			break;
		default:
			break;
		}
		spin_lock(&(DPEInfo.SpinLockDPE));
		g_u4EnableClockCount++;
		spin_unlock(&(DPEInfo.SpinLockDPE));
	} else {		/* Disable clock. */

		/* LOG_DBG("Dpe clock disabled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		spin_lock(&(DPEInfo.SpinLockDPE));
		g_u4EnableClockCount--;
		spin_unlock(&(DPEInfo.SpinLockDPE));
		switch (g_u4EnableClockCount) {
		case 0:
#ifndef __DPE_EP_NO_CLKMGR__
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
			    DPE_Disable_Unprepare_ccf_clock();
#else
			/* do disable clock */
			disable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
			disable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
			disable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
			disable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
			disable_clock(MT_CG_IMAGE_CAM_SV, "CAMERA");
			/* disable_clock(MT_CG_IMAGE_FD, "CAMERA"); */
			disable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
			disable_clock(MT_CG_DDPE0_SMI_COMMON, "CAMERA");
#endif				/* #if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) */
#endif
			break;
		default:
			break;
		}
	}
}

/*******************************************************************************
*
********************************************************************************/
static inline void DPE_Reset(void)
{
	LOG_DBG("- E.");

	LOG_DBG(" DPE Reset start!\n");
	spin_lock(&(DPEInfo.SpinLockDPERef));

	if (DPEInfo.UserCount > 1) {
		spin_unlock(&(DPEInfo.SpinLockDPERef));
		LOG_DBG("Curr UserCount(%d) users exist", DPEInfo.UserCount);
	} else {
		spin_unlock(&(DPEInfo.SpinLockDPERef));

		/* Reset DPE flow */
		DPE_WR32(DPE_RST_REG, 0x1);
		while ((DPE_RD32(DPE_RST_REG) & 0x02) != 0x2)
			LOG_DBG("DPE resetting...\n");

		DPE_WR32(DPE_RST_REG, 0x11);
		DPE_WR32(DPE_RST_REG, 0x10);
		DPE_WR32(DPE_RST_REG, 0x0);
		DPE_WR32(DPE_DVE_START_REG, 0);
		DPE_WR32(DPE_WMFE_START_REG, 0);
		LOG_DBG(" DPE Reset end!\n");
	}

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_ReadReg(DPE_REG_IO_STRUCT *pRegIo)
{
	MUINT32 i;
	MINT32 Ret = 0;
	/*  */
	DPE_REG_STRUCT reg;
	/* MUINT32* pData = (MUINT32*)pRegIo->Data; */
	DPE_REG_STRUCT *pData = (DPE_REG_STRUCT *) pRegIo->pData;

	for (i = 0; i < pRegIo->Count; i++) {
		if (get_user(reg.Addr, (MUINT32 *) &pData->Addr) != 0) {
			LOG_ERR("get_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		/*  */
		if ((ISP_DPE_BASE + reg.Addr >= ISP_DPE_BASE)
		    && (ISP_DPE_BASE + reg.Addr < (ISP_DPE_BASE + DPE_REG_RANGE))) {
			reg.Val = DPE_RD32(ISP_DPE_BASE + reg.Addr);
		} else {
			LOG_ERR("Wrong address(0x%p)", (ISP_DPE_BASE + reg.Addr));
			reg.Val = 0;
		}
		/*  */
		/* printk("[KernelRDReg]addr(0x%x),value()0x%x\n",DPE_ADDR_CAMINF + reg.Addr,reg.Val); */

		if (put_user(reg.Val, (MUINT32 *) &(pData->Val)) != 0) {
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
********************************************************************************/
/* Can write sensor's test model only, if need write to other modules, need modify current code flow */
static MINT32 DPE_WriteRegToHw(DPE_REG_STRUCT *pReg, MUINT32 Count)
{
	MINT32 Ret = 0;
	MUINT32 i;
	MBOOL dbgWriteReg;

	/* Use local variable to store DPEInfo.DebugMask & DPE_DBG_WRITE_REG for saving lock time */
	spin_lock(&(DPEInfo.SpinLockDPE));
	dbgWriteReg = DPEInfo.DebugMask & DPE_DBG_WRITE_REG;
	spin_unlock(&(DPEInfo.SpinLockDPE));

	/*  */
	if (dbgWriteReg)
		LOG_DBG("- E.\n");

	/*  */
	for (i = 0; i < Count; i++) {
		if (dbgWriteReg) {
			LOG_DBG("Addr(0x%lx), Val(0x%x)\n",
				(unsigned long)(ISP_DPE_BASE + pReg[i].Addr),
				(MUINT32) (pReg[i].Val));
		}

		if (((ISP_DPE_BASE + pReg[i].Addr) < (ISP_DPE_BASE + DPE_REG_RANGE))) {
			DPE_WR32(ISP_DPE_BASE + pReg[i].Addr, pReg[i].Val);
		} else {
			LOG_ERR("wrong address(0x%lx)\n",
				(unsigned long)(ISP_DPE_BASE + pReg[i].Addr));
		}
	}

	/*  */
	return Ret;
}



/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_WriteReg(DPE_REG_IO_STRUCT *pRegIo)
{
	MINT32 Ret = 0;
	/* MUINT8* pData = NULL; */
	DPE_REG_STRUCT *pData = NULL;
	/*  */
	if (DPEInfo.DebugMask & DPE_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n", (pRegIo->pData), (pRegIo->Count));

	/* pData = (MUINT8*)kmalloc((pRegIo->Count)*sizeof(DPE_REG_STRUCT), GFP_ATOMIC); */
	pData = kmalloc((pRegIo->Count) * sizeof(DPE_REG_STRUCT), GFP_ATOMIC);
	if (pData == NULL) {
		LOG_ERR("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)\n", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
		goto EXIT;
	}
	/*  */
	if ((pRegIo->pData == NULL) || (pRegIo->Count == 0)) {
		LOG_ERR("ERROR: pRegIo->pData is NULL or Count:%d\n", pRegIo->Count);
		Ret = -EFAULT;
		goto EXIT;
	}
	if (copy_from_user
	    (pData, (void __user *)(pRegIo->pData), pRegIo->Count * sizeof(DPE_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = DPE_WriteRegToHw(pData, pRegIo->Count);
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
********************************************************************************/
static MINT32 DPE_WaitIrq(DPE_WAIT_IRQ_STRUCT *WaitIrq)
{

	MINT32 Ret = 0;
	MINT32 Timeout = WaitIrq->Timeout;
	DPE_PROCESS_ID_ENUM whichReq = DPE_PROCESS_ID_NONE;

	/*MUINT32 i;*/
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */
	MUINT32 irqStatus;
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
	if (DPEInfo.DebugMask & DPE_DBG_INT) {
		if (WaitIrq->Status & DPEInfo.IrqInfo.Mask[WaitIrq->Type]) {
			if (WaitIrq->UserKey > 0) {
				LOG_DBG("+WaitIrq Clr(%d),Type(%d),Sta(0x%08X),Timeout(%d),user(%d),PID(%d)\n",
				     WaitIrq->Clear, WaitIrq->Type, WaitIrq->Status,
				     WaitIrq->Timeout, WaitIrq->UserKey, WaitIrq->ProcessID);
			}
		}
	}


	/* 1. wait type update */
	if (WaitIrq->Clear == DPE_IRQ_CLEAR_STATUS) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		DPEInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		return Ret;
	}

	if (WaitIrq->Clear == DPE_IRQ_CLEAR_WAIT) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		if (DPEInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
			DPEInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);

		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
	} else if (WaitIrq->Clear == DPE_IRQ_CLEAR_ALL) {
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);

		DPEInfo.IrqInfo.Status[WaitIrq->Type] = 0;
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
	}
	/* DPE_IRQ_WAIT_CLEAR ==> do nothing */


	/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
	spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
	irqStatus = DPEInfo.IrqInfo.Status[WaitIrq->Type];
	spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);

	if (WaitIrq->Status & DPE_DVE_INT_ST) {
		whichReq = DPE_PROCESS_ID_DVE;
	} else if (WaitIrq->Status & DPE_WMFE_INT_ST) {
		whichReq = DPE_PROCESS_ID_WMFE;
	} else {
		LOG_ERR("No Such Stats can be waited!! irq Type/User/Sts/Pid(0x%x/%d/0x%x/%d)\n",
			WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, WaitIrq->ProcessID);
	}


#ifdef DPE_WAITIRQ_LOG
	LOG_INF("before wait_event! Timeout(%d)Clear(%d),Type(%d),IrqSta(0x%08X), WaitSta(0x%08X)\n",
		WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus, WaitIrq->Status);
	LOG_INF("urKey(%d),whReq(%d),PID(%d)\n", WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
	LOG_INF("DveIrqCnt(0x%08X),WmfeIrqCnt(0x%08X),WriteReqIdx(0x%08X),ReadReqIdx(0x%08X)\n",
	DPEInfo.IrqInfo.DveIrqCnt, DPEInfo.IrqInfo.WmfeIrqCnt, DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);
#endif

	/* 2. start to wait signal */
	Timeout = wait_event_interruptible_timeout(DPEInfo.WaitQueueHead,
						   DPE_GetIRQState(WaitIrq->Type, WaitIrq->UserKey,
								   WaitIrq->Status, whichReq,
								   WaitIrq->ProcessID),
						   DPE_MsToJiffies(WaitIrq->Timeout));

	/* check if user is interrupted by system signal */
	if ((Timeout != 0)
	    &&
	    (!DPE_GetIRQState
	     (WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, whichReq, WaitIrq->ProcessID))) {
		LOG_DBG("waked up by sys. signal,ret(%d),irq Type/User/Sts/whReq/Pid(0x%x/%d/0x%x/%d/%d)\n",
		     Timeout, WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, whichReq,
		     WaitIrq->ProcessID);
		Ret = -ERESTARTSYS;	/* actually it should be -ERESTARTSYS */
		goto EXIT;
	}
	/* timeout */
	if (Timeout == 0) {
		/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = DPEInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);

		LOG_ERR("ERRRR Timeout!Timeout(%d)Clear(%d),Type(%d),IrqSta(0x%08X), WaitSta(0x%08X)\n",
			WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus, WaitIrq->Status);
		LOG_ERR("urKey(%d),whReq(%d),PID(%d)\n", WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
		LOG_ERR("DveIrqCnt(0x%08X),WmfeIrqCnt(0x%08X),WriteReqIdx(0x%08X),ReadReqIdx(0x%08X)\n",
		DPEInfo.IrqInfo.DveIrqCnt, DPEInfo.IrqInfo.WmfeIrqCnt, DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);

		if (WaitIrq->bDumpReg)
			DPE_DumpReg();

		Ret = -EFAULT;
		goto EXIT;
	} else {
		/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("DPE_WaitIrq");
#endif

		spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = DPEInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);

		if (WaitIrq->Clear == DPE_IRQ_WAIT_CLEAR) {
			spin_lock_irqsave(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
#ifdef DPE_USE_GCE

#ifdef DPE_MULTIPROCESS_TIMEING_ISSUE
			DPEInfo.ReadReqIdx =
			    (DPEInfo.ReadReqIdx + 1) % _SUPPORT_MAX_DPE_FRAME_REQUEST_;
			/* actually, it doesn't happen the timging issue!! */
			/* wake_up_interruptible(&DPEInfo.WaitQueueHead); */
#endif
			if (WaitIrq->Status & DPE_DVE_INT_ST) {
				DPEInfo.IrqInfo.DveIrqCnt--;
				if (DPEInfo.IrqInfo.DveIrqCnt == 0)
					DPEInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
			} else if (WaitIrq->Status & DPE_WMFE_INT_ST) {
				DPEInfo.IrqInfo.WmfeIrqCnt--;
				if (DPEInfo.IrqInfo.WmfeIrqCnt == 0)
					DPEInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
			} else {
				LOG_ERR("DPE_IRQ_WAIT_CLEAR Error, Type(%d), WaitStatus(0x%08X)",
					WaitIrq->Type, WaitIrq->Status);
			}
#else
			if (DPEInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
				DPEInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
#endif
			spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[WaitIrq->Type]), flags);
		}

#ifdef DPE_WAITIRQ_LOG
		LOG_INF("no Timeout!Timeout(%d)Clear(%d),Type(%d),IrqSta(0x%08X), WaitSta(0x%08X)\n",
			WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus, WaitIrq->Status);
		LOG_INF("urKey(%d),whReq(%d),PID(%d)\n", WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
		LOG_INF("DveIrqCnt(0x%08X),WmfeIrqCnt(0x%08X),WriteReqIdx(0x%08X),ReadReqIdx(0x%08X)\n",
		DPEInfo.IrqInfo.DveIrqCnt, DPEInfo.IrqInfo.WmfeIrqCnt, DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);

#endif

#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif

	}


EXIT:


	return Ret;
}


/*******************************************************************************
*
********************************************************************************/
static long DPE_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	MINT32 Ret = 0;

	/*MUINT32 pid = 0;*/
	DPE_REG_IO_STRUCT RegIo;
	DPE_WAIT_IRQ_STRUCT IrqInfo;
	DPE_CLEAR_IRQ_STRUCT ClearIrq;
	DPE_DVERequest dpe_DveReq;
	DPE_WMFERequest dpe_WmfeReq;
	MINT32 DveWriteIdx = 0;
	MINT32 WmfeWriteIdx = 0;
	int idx;
	DPE_USER_INFO_STRUCT *pUserInfo;
	int dequeNum;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */



	/*  */
	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		return -EFAULT;
	}
	/*  */
	pUserInfo = (DPE_USER_INFO_STRUCT *) (pFile->private_data);
	/*  */
	switch (Cmd) {
	case DPE_RESET:
		{
			spin_lock(&(DPEInfo.SpinLockDPE));
			DPE_Reset();
			spin_unlock(&(DPEInfo.SpinLockDPE));
			break;
		}

		/*  */
	case DPE_DUMP_REG:
		{
			Ret = DPE_DumpReg();
			break;
		}
	case DPE_DUMP_ISR_LOG:
		{
			MUINT32 currentPPB = m_CurrentPPB;

			spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]), flags);
			m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
			spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
					       flags);

			IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DPE_ST, currentPPB, _LOG_INF);
			IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DPE_ST, currentPPB, _LOG_ERR);
			break;
		}
	case DPE_READ_REGISTER:
		{
			if (copy_from_user(&RegIo, (void *)Param, sizeof(DPE_REG_IO_STRUCT)) == 0) {
				/* 2nd layer behavoir of copy from user is implemented in DPE_ReadReg(...) */
				Ret = DPE_ReadReg(&RegIo);
			} else {
				LOG_ERR("DPE_READ_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_WRITE_REGISTER:
		{
			if (copy_from_user(&RegIo, (void *)Param, sizeof(DPE_REG_IO_STRUCT)) == 0) {
				/* 2nd layer behavoir of copy from user is implemented in DPE_WriteReg(...) */
				Ret = DPE_WriteReg(&RegIo);
			} else {
				LOG_ERR("DPE_WRITE_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_WAIT_IRQ:
		{
			if (copy_from_user(&IrqInfo, (void *)Param, sizeof(DPE_WAIT_IRQ_STRUCT)) ==
			    0) {
				/*  */
				if ((IrqInfo.Type >= DPE_IRQ_TYPE_AMOUNT) || (IrqInfo.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)", IrqInfo.Type);
					goto EXIT;
				}

				if ((IrqInfo.UserKey >= IRQ_USER_NUM_MAX) || (IrqInfo.UserKey < 0)) {
					LOG_ERR("invalid userKey(%d), max(%d), force userkey = 0\n",
						IrqInfo.UserKey, IRQ_USER_NUM_MAX);
					IrqInfo.UserKey = 0;
				}

				LOG_INF
				    ("IRQ clear(%d), type(%d), userKey(%d), timeout(%d), status(%d)\n",
				     IrqInfo.Clear, IrqInfo.Type, IrqInfo.UserKey, IrqInfo.Timeout,
				     IrqInfo.Status);
				IrqInfo.ProcessID = pUserInfo->Pid;
				Ret = DPE_WaitIrq(&IrqInfo);

				if (copy_to_user
				    ((void *)Param, &IrqInfo, sizeof(DPE_WAIT_IRQ_STRUCT)) != 0) {
					LOG_ERR("copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("DPE_WAIT_IRQ copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_CLEAR_IRQ:
		{
			if (copy_from_user(&ClearIrq, (void *)Param, sizeof(DPE_CLEAR_IRQ_STRUCT))
			    == 0) {
				LOG_DBG("DPE_CLEAR_IRQ Type(%d)", ClearIrq.Type);

				if ((ClearIrq.Type >= DPE_IRQ_TYPE_AMOUNT) || (ClearIrq.Type < 0)) {
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

				LOG_DBG("DPE_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)\n",
					ClearIrq.Type, ClearIrq.Status,
					DPEInfo.IrqInfo.Status[ClearIrq.Type]);
				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[ClearIrq.Type]), flags);
				DPEInfo.IrqInfo.Status[ClearIrq.Type] &= (~ClearIrq.Status);
				spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[ClearIrq.Type]),
						       flags);
			} else {
				LOG_ERR("DPE_CLEAR_IRQ copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_DVE_ENQUE_REQ:
		{
		MINT32 WIdx;
		MINT32 FWRIdx;

			if (copy_from_user(&dpe_DveReq, (void *)Param, sizeof(DPE_DVERequest)) == 0) {
				LOG_DBG("DVE_ENQNUE_NUM:%d, pid:%d\n", dpe_DveReq.m_ReqNum, pUserInfo->Pid);
				if (dpe_DveReq.m_ReqNum > _SUPPORT_MAX_DPE_FRAME_REQUEST_) {
					LOG_ERR("DVE Enque Num is bigger than enqueNum:%d\n",
						dpe_DveReq.m_ReqNum);
					Ret = -EFAULT;
					goto EXIT;
				}
				if (copy_from_user
				    (g_DveEnqueReq_Struct.DveFrameConfig, (void *)dpe_DveReq.m_pDpeConfig,
				     dpe_DveReq.m_ReqNum * sizeof(DPE_DVEConfig)) != 0) {
					LOG_ERR("copy DVEConfig from request is fail!!\n");
					Ret = -EFAULT;
					goto EXIT;
				}

				mutex_lock(&gDpeDveMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]), flags);
				WIdx = g_DVE_RequestRing.WriteIdx;
				FWRIdx = g_DVE_RequestRing.DVEReq_Struct[WIdx].FrameWRIdx;
				if (DPE_REQUEST_STATE_EMPTY ==
				    g_DVE_RequestRing.DVEReq_Struct[WIdx].RequestState) {
					g_DVE_RequestRing.DVEReq_Struct[WIdx].processID = pUserInfo->Pid;
					g_DVE_RequestRing.DVEReq_Struct[WIdx].enqueReqNum = dpe_DveReq.m_ReqNum;
					for (idx = 0; idx < dpe_DveReq.m_ReqNum; idx++) {
						g_DVE_RequestRing.DVEReq_Struct[WIdx].DveFrameStatus[FWRIdx]
							= DPE_FRAME_STATUS_ENQUE;
						memcpy(&g_DVE_RequestRing.DVEReq_Struct[WIdx].DveFrameConfig[FWRIdx++],
						&g_DveEnqueReq_Struct.DveFrameConfig[idx], sizeof(DPE_DVEConfig));
					}
					g_DVE_RequestRing.DVEReq_Struct[WIdx].RequestState = DPE_REQUEST_STATE_PENDING;
					DveWriteIdx = WIdx;
					g_DVE_RequestRing.WriteIdx = (WIdx + 1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					LOG_INF("DVE request enque done!\n");
				} else {
					LOG_ERR("No DVE Buf! WriteIdx(%d),ReqSta(%d),FrameWRIdx(%d),enqReqNum(%d)\n",
					     g_DVE_RequestRing.WriteIdx,
					     g_DVE_RequestRing.DVEReq_Struct[WIdx].RequestState,
					     g_DVE_RequestRing.DVEReq_Struct[WIdx].FrameWRIdx,
					     g_DVE_RequestRing.DVEReq_Struct[WIdx].enqueReqNum);
				}
				g_DVE_RequestRing.DVEReq_Struct[WIdx].FrameWRIdx = FWRIdx;
				spin_unlock_irqrestore(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						       flags);
				LOG_DBG("ConfigDVE Request!!\n");
				ConfigDVERequest(DveWriteIdx);

				mutex_unlock(&gDpeDveMutex);
			} else {
				LOG_ERR("DPE_DVE_ENQUE copy_from_user failed\n");
				Ret = -EFAULT;
			}
		}

		break;
	case DPE_DVE_DEQUE_REQ:
		{
			MINT32 ReadIdx;
			MINT32 FrameRDIdx;

			if (copy_from_user(&dpe_DveReq, (void *)Param, sizeof(DPE_DVERequest)) == 0) {
				mutex_lock(&gDpeDveDequeMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						  flags);
				ReadIdx = g_DVE_RequestRing.ReadIdx;
				FrameRDIdx = g_DVE_RequestRing.DVEReq_Struct[ReadIdx].FrameRDIdx;
				if (DPE_REQUEST_STATE_FINISHED ==
				    g_DVE_RequestRing.DVEReq_Struct[ReadIdx].RequestState) {
					dequeNum = g_DVE_RequestRing.DVEReq_Struct[ReadIdx].enqueReqNum;
					LOG_DBG("DVE DEQUE_NUM(%d)\n", dequeNum);
				} else {
					dequeNum = 0;
					LOG_ERR("DEQ_NUM No Buf!,ReadIdx(%d),ReqSta(%d),FrameRDIdx(%d),enqReqNum(%d)\n",
					     ReadIdx,
					     g_DVE_RequestRing.DVEReq_Struct[ReadIdx].RequestState,
					     g_DVE_RequestRing.DVEReq_Struct[ReadIdx].FrameRDIdx,
					     g_DVE_RequestRing.DVEReq_Struct[ReadIdx].enqueReqNum);
				}
				dpe_DveReq.m_ReqNum = dequeNum;
				for (idx = 0; idx < dequeNum; idx++) {
					if (DPE_FRAME_STATUS_FINISHED ==
					    g_DVE_RequestRing.DVEReq_Struct[ReadIdx].DveFrameStatus[FrameRDIdx]) {
						memcpy(&g_DveDequeReq_Struct.DveFrameConfig[idx],
						&g_DVE_RequestRing.DVEReq_Struct[ReadIdx].DveFrameConfig[FrameRDIdx],
						sizeof(DPE_DVEConfig));
						g_DVE_RequestRing.DVEReq_Struct[ReadIdx].DveFrameStatus[FrameRDIdx++] =
						    DPE_FRAME_STATUS_EMPTY;
					} else {
						LOG_ERR("DVE!idx(%d),deqNum(%d),ReadIdx,(%d),FRDIdx(%d),DveFSta(%d)\n",
						idx, dequeNum, g_DVE_RequestRing.ReadIdx,
						g_DVE_RequestRing.DVEReq_Struct[ReadIdx].FrameRDIdx,
						g_DVE_RequestRing.DVEReq_Struct[ReadIdx].DveFrameStatus[FrameRDIdx]);
					}
				}
				g_DVE_RequestRing.DVEReq_Struct[ReadIdx].FrameRDIdx = FrameRDIdx;
				g_DVE_RequestRing.DVEReq_Struct[ReadIdx].RequestState = DPE_REQUEST_STATE_EMPTY;
				g_DVE_RequestRing.DVEReq_Struct[ReadIdx].FrameWRIdx = 0;
				g_DVE_RequestRing.DVEReq_Struct[ReadIdx].FrameRDIdx = 0;
				g_DVE_RequestRing.DVEReq_Struct[ReadIdx].enqueReqNum = 0;
				g_DVE_RequestRing.ReadIdx = (ReadIdx + 1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
				LOG_INF("DVE Request ReadIdx(%d)\n", g_DVE_RequestRing.ReadIdx);


				spin_unlock_irqrestore(&
						       (DPEInfo.
							SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						       flags);

				mutex_unlock(&gDpeDveDequeMutex);

				if (copy_to_user
				    ((void *)dpe_DveReq.m_pDpeConfig,
				     &g_DveDequeReq_Struct.DveFrameConfig[0],
				     dequeNum * sizeof(DPE_DVEConfig)) != 0) {
					LOG_ERR
					    ("DPE_CMD_DVE_DEQUE_REQ copy_to_user frameconfig failed\n");
					Ret = -EFAULT;
				}
				if (copy_to_user((void *)Param, &dpe_DveReq, sizeof(DPE_DVERequest))
				    != 0) {
					LOG_ERR("DPE_CMD_DVE_DEQUE_REQ copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("DPE_CMD_DVE_DEQUE_REQ copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case DPE_WMFE_ENQUE_REQ:
		{
			MINT32 WIdx;
			MINT32 FWRIdx;

			if (copy_from_user(&dpe_WmfeReq, (void *)Param, sizeof(DPE_WMFERequest)) ==
			    0) {
				LOG_DBG("WMFE_ENQNUE_NUM:%d, pid:%d\n", dpe_WmfeReq.m_ReqNum,
					pUserInfo->Pid);
				if (dpe_WmfeReq.m_ReqNum > _SUPPORT_MAX_DPE_FRAME_REQUEST_) {
					LOG_ERR("WMFE Enque Num is bigger than enqueNum:%d\n",
						dpe_WmfeReq.m_ReqNum);
					Ret = -EFAULT;
					goto EXIT;
				}
				if (copy_from_user
				    (g_WmfeEnqueReq_Struct.WmfeFrameConfig,
				     (void *)dpe_WmfeReq.m_pWmfeConfig,
				     dpe_WmfeReq.m_ReqNum * sizeof(DPE_WMFEConfig)) != 0) {
					LOG_ERR("copy WMFEConfig from request is fail!!\n");
					Ret = -EFAULT;
					goto EXIT;
				}

				mutex_lock(&gDpeWmfeMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						  flags);
				WIdx = g_WMFE_ReqRing.WriteIdx;
				FWRIdx = g_WMFE_ReqRing.WMFEReq_Struct[WIdx].FrameWRIdx;
				if (DPE_REQUEST_STATE_EMPTY ==
				    g_WMFE_ReqRing.WMFEReq_Struct[WIdx].RequestState) {
					g_WMFE_ReqRing.WMFEReq_Struct[WIdx].processID = pUserInfo->Pid;
					g_WMFE_ReqRing.WMFEReq_Struct[WIdx].enqueReqNum = dpe_WmfeReq.m_ReqNum;

					for (idx = 0; idx < dpe_WmfeReq.m_ReqNum; idx++) {
						g_WMFE_ReqRing.
						WMFEReq_Struct[WIdx].WmfeFrameStatus[FWRIdx] = DPE_FRAME_STATUS_ENQUE;
						memcpy(&g_WMFE_ReqRing.WMFEReq_Struct[WIdx].WmfeFrameConfig[FWRIdx++],
						&g_WmfeEnqueReq_Struct.WmfeFrameConfig[idx], sizeof(DPE_WMFEConfig));
					}
					g_WMFE_ReqRing.WMFEReq_Struct[WIdx].FrameWRIdx = FWRIdx;
					g_WMFE_ReqRing.WMFEReq_Struct[WIdx].RequestState = DPE_REQUEST_STATE_PENDING;
					WmfeWriteIdx = WIdx;
					g_WMFE_ReqRing.WriteIdx = (WIdx + 1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
					LOG_INF("WMFE request enque done!!\n");
				} else {
					LOG_ERR("No Empty WMFE Buf!WriteIdx(%d),ReqSta(%d),FWRIdx(%d),enqReqNum(%d)\n",
					     WIdx,
					     g_WMFE_ReqRing.WMFEReq_Struct[WIdx].RequestState,
					     g_WMFE_ReqRing.WMFEReq_Struct[WIdx].FrameWRIdx,
					     g_WMFE_ReqRing.WMFEReq_Struct[WIdx].enqueReqNum);
				}
				g_WMFE_ReqRing.WMFEReq_Struct[WIdx].FrameWRIdx = FWRIdx;
				spin_unlock_irqrestore(&
						       (DPEInfo.
							SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						       flags);
				LOG_DBG("ConfigWMFE Request!!\n");
				ConfigWMFERequest(WmfeWriteIdx);

				mutex_unlock(&gDpeWmfeMutex);
			} else {
				LOG_ERR("DPE_DVE_ENQUE copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	case DPE_WMFE_DEQUE_REQ:
		{
			MINT32 FrameRDIdx;
			MINT32 ReadIdx;

			if (copy_from_user(&dpe_WmfeReq, (void *)Param, sizeof(DPE_WMFERequest)) ==
			    0) {
				mutex_lock(&gDpeWmfeDequeMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						  flags);
				ReadIdx = g_WMFE_ReqRing.ReadIdx;
				FrameRDIdx = g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].FrameRDIdx;
				if (DPE_REQUEST_STATE_FINISHED ==
				    g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].RequestState) {
					dequeNum = g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].enqueReqNum;
					LOG_DBG("WMFE_DEQUE_NUM(%d)\n", dequeNum);
				} else {
					dequeNum = 0;
					LOG_ERR("Deq No WMFE Buf,ReadIdx(%d),ReqSta(%d),FRDIdx(%d),enqReqNum(%d)\n",
					     ReadIdx,
					     g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].RequestState,
					     FrameRDIdx,
					     g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].enqueReqNum);
				}
				dpe_WmfeReq.m_ReqNum = dequeNum;
				for (idx = 0; idx < dequeNum; idx++) {
					if (DPE_FRAME_STATUS_FINISHED ==
					    g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].WmfeFrameStatus[FrameRDIdx]) {
						memcpy(&g_WmfeDequeReq_Struct.WmfeFrameConfig[idx],
						&g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].WmfeFrameConfig[FrameRDIdx],
						sizeof(DPE_WMFEConfig));
						g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].WmfeFrameStatus[FrameRDIdx++]
						= DPE_FRAME_STATUS_EMPTY;
					} else {
						LOG_ERR("WMFE!idx(%d),deqNum(%d),ReadIdx,(%d),FRDIdx(%d),FSts(%d)\n",
						idx, dequeNum, ReadIdx, FrameRDIdx,
						g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].WmfeFrameStatus[FrameRDIdx]);
					}
				}
				g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].FrameRDIdx = FrameRDIdx;
				g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].RequestState = DPE_REQUEST_STATE_EMPTY;
				g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].FrameWRIdx = 0;
				g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].FrameRDIdx = 0;
				g_WMFE_ReqRing.WMFEReq_Struct[ReadIdx].enqueReqNum = 0;
				g_WMFE_ReqRing.ReadIdx = (ReadIdx + 1) % _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_;
				LOG_DBG("WMFE Request ReadIdx(%d)\n", g_WMFE_ReqRing.ReadIdx);

				spin_unlock_irqrestore(&
						       (DPEInfo.
							SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]),
						       flags);

				mutex_unlock(&gDpeWmfeDequeMutex);

				if (copy_to_user
				    ((void *)dpe_WmfeReq.m_pWmfeConfig,
				     &g_WmfeDequeReq_Struct.WmfeFrameConfig[0],
				     dequeNum * sizeof(DPE_WMFEConfig)) != 0) {
					LOG_ERR
					    ("DPE_WMFE_DEQUE_REQ copy_to_user frameconfig failed\n");
					Ret = -EFAULT;
				}
				if (copy_to_user
				    ((void *)Param, &dpe_WmfeReq, sizeof(DPE_WMFERequest)) != 0) {
					LOG_ERR("DPE_WMFE_DEQUE_REQ copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("DPE_CMD_WMFE_DEQUE_REQ copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	default:
		{
			LOG_ERR("Unknown Cmd(%d)", Cmd);
			LOG_ERR("Fail, Cmd(%d), Dir(%d), Type(%d), Nr(%d),Size(%d)\n", Cmd, _IOC_DIR(Cmd),
				_IOC_TYPE(Cmd), _IOC_NR(Cmd), _IOC_SIZE(Cmd));
			Ret = -EPERM;
			break;
		}
	}
	/*  */
EXIT:
	if (Ret != 0) {
		LOG_ERR("Fail, Cmd(%d), Pid(%d), (process, pid, tgid)=(%s, %d, %d)", Cmd,
			pUserInfo->Pid, current->comm, current->pid, current->tgid);
	}
	/*  */
	return Ret;
}

#ifdef CONFIG_COMPAT

/*******************************************************************************
*
********************************************************************************/
static int compat_get_DPE_read_register_data(compat_DPE_REG_IO_STRUCT __user *data32,
					     DPE_REG_IO_STRUCT __user *data)
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

static int compat_put_DPE_read_register_data(compat_DPE_REG_IO_STRUCT __user *data32,
					     DPE_REG_IO_STRUCT __user *data)
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

static int compat_get_DPE_dve_enque_req_data(compat_DPE_DVERequest __user *data32,
					     DPE_DVERequest __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pDpeConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pDpeConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_DPE_dve_enque_req_data(compat_DPE_DVERequest __user *data32,
					     DPE_DVERequest __user *data)
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


static int compat_get_DPE_dve_deque_req_data(compat_DPE_DVERequest __user *data32,
					     DPE_DVERequest __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pDpeConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pDpeConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_DPE_dve_deque_req_data(compat_DPE_DVERequest __user *data32,
					     DPE_DVERequest __user *data)
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

static int compat_get_DPE_wmfe_enque_req_data(compat_DPE_WMFERequest __user *data32,
					      DPE_WMFERequest __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pWmfeConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pWmfeConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_DPE_wmfe_enque_req_data(compat_DPE_WMFERequest __user *data32,
					      DPE_WMFERequest __user *data)
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


static int compat_get_DPE_wmfe_deque_req_data(compat_DPE_WMFERequest __user *data32,
					      DPE_WMFERequest __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pWmfeConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pWmfeConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_DPE_wmfe_deque_req_data(compat_DPE_WMFERequest __user *data32,
					      DPE_WMFERequest __user *data)
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

static long DPE_ioctl_compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;


	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		LOG_ERR("no f_op !!!\n");
		return -ENOTTY;
	}
	switch (cmd) {
	case COMPAT_DPE_READ_REGISTER:
		{
			compat_DPE_REG_IO_STRUCT __user *data32;
			DPE_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_DPE_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_get_DPE_read_register_data error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, DPE_READ_REGISTER,
						       (unsigned long)data);
			err = compat_put_DPE_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_put_DPE_read_register_data error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_DPE_WRITE_REGISTER:
		{
			compat_DPE_REG_IO_STRUCT __user *data32;
			DPE_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_DPE_read_register_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_WRITE_REGISTER error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, DPE_WRITE_REGISTER,
						       (unsigned long)data);
			return ret;
		}
	case COMPAT_DPE_DVE_ENQUE_REQ:
		{
			compat_DPE_DVERequest __user *data32;
			DPE_DVERequest __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_DPE_dve_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_DVE_ENQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, DPE_DVE_ENQUE_REQ,
						       (unsigned long)data);
			err = compat_put_DPE_dve_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_DVE_ENQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_DPE_DVE_DEQUE_REQ:
		{
			compat_DPE_DVERequest __user *data32;
			DPE_DVERequest __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_DPE_dve_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_DVE_DEQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, DPE_DVE_DEQUE_REQ,
						       (unsigned long)data);
			err = compat_put_DPE_dve_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_DVE_DEQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}

	case COMPAT_DPE_WMFE_ENQUE_REQ:
		{
			compat_DPE_WMFERequest __user *data32;
			DPE_WMFERequest __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_DPE_wmfe_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_WMFE_ENQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, DPE_WMFE_ENQUE_REQ,
						       (unsigned long)data);
			err = compat_put_DPE_wmfe_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_WMFE_ENQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_DPE_WMFE_DEQUE_REQ:
		{
			compat_DPE_WMFERequest __user *data32;
			DPE_WMFERequest __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_DPE_wmfe_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_WMFE_DEQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, DPE_WMFE_DEQUE_REQ,
						       (unsigned long)data);
			err = compat_put_DPE_wmfe_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_DPE_WMFE_DEQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}

	case DPE_WAIT_IRQ:
	case DPE_CLEAR_IRQ:	/* structure (no pointer) */
	case DPE_RESET:
	case DPE_DUMP_REG:
	case DPE_DUMP_ISR_LOG:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return DPE_ioctl(filep, cmd, arg); */
	}
}

#endif

/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_open(struct inode *pInode, struct file *pFile)
{
	MINT32 Ret = 0;
	MUINT32 i, j;
	/*int q = 0, p = 0;*/
	DPE_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("- E. UserCount: %d.", DPEInfo.UserCount);


	/*  */
	spin_lock(&(DPEInfo.SpinLockDPERef));

	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(DPE_USER_INFO_STRUCT), GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (DPE_USER_INFO_STRUCT *) pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
	if (DPEInfo.UserCount > 0) {
		DPEInfo.UserCount++;
		spin_unlock(&(DPEInfo.SpinLockDPERef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			DPEInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else {
		DPEInfo.UserCount++;
		spin_unlock(&(DPEInfo.SpinLockDPERef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), first user",
			DPEInfo.UserCount, current->comm, current->pid, current->tgid);
	}

	/* do wait queue head init when re-enter in camera */
	/*  */
	for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		g_DVE_RequestRing.DVEReq_Struct[i].processID = 0x0;
		g_DVE_RequestRing.DVEReq_Struct[i].callerID = 0x0;
		g_DVE_RequestRing.DVEReq_Struct[i].enqueReqNum = 0x0;
		/* g_DVE_RequestRing.DVEReq_Struct[i].enqueIdx = 0x0; */
		g_DVE_RequestRing.DVEReq_Struct[i].RequestState = DPE_REQUEST_STATE_EMPTY;
		g_DVE_RequestRing.DVEReq_Struct[i].FrameWRIdx = 0x0;
		g_DVE_RequestRing.DVEReq_Struct[i].FrameRDIdx = 0x0;
		/* WMFE */
		g_WMFE_ReqRing.WMFEReq_Struct[i].processID = 0x0;
		g_WMFE_ReqRing.WMFEReq_Struct[i].callerID = 0x0;
		g_WMFE_ReqRing.WMFEReq_Struct[i].enqueReqNum = 0x0;
		/* g_WMFE_ReqRing.WMFEReq_Struct[i].enqueIdx = 0x0; */
		g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState = DPE_REQUEST_STATE_EMPTY;
		g_WMFE_ReqRing.WMFEReq_Struct[i].FrameWRIdx = 0x0;
		g_WMFE_ReqRing.WMFEReq_Struct[i].FrameRDIdx = 0x0;
		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_; j++) {
			g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j] =
			    DPE_FRAME_STATUS_EMPTY;
			g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j] =
			    DPE_FRAME_STATUS_EMPTY;
		}

	}
	g_DVE_RequestRing.WriteIdx = 0x0;
	g_DVE_RequestRing.ReadIdx = 0x0;
	g_DVE_RequestRing.HWProcessIdx = 0x0;
	g_WMFE_ReqRing.WriteIdx = 0x0;
	g_WMFE_ReqRing.ReadIdx = 0x0;
	g_WMFE_ReqRing.HWProcessIdx = 0x0;

	/* Enable clock */
	DPE_EnableClock(MTRUE);
	LOG_DBG("DPE open g_u4EnableClockCount: %d", g_u4EnableClockCount);
	/*  */

	for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++)
		DPEInfo.IrqInfo.Status[i] = 0;

	for (i = 0; i < _SUPPORT_MAX_DPE_FRAME_REQUEST_; i++)
		DPEInfo.ProcessID[i] = 0;

	DPEInfo.WriteReqIdx = 0;
	DPEInfo.ReadReqIdx = 0;
	DPEInfo.IrqInfo.DveIrqCnt = 0;
	DPEInfo.IrqInfo.WmfeIrqCnt = 0;


#ifdef KERNEL_LOG
    /* In EP, Add DPE_DBG_WRITE_REG for debug. Should remove it after EP */
	DPEInfo.DebugMask = (DPE_DBG_INT | DPE_DBG_DBGLOG | DPE_DBG_WRITE_REG);
#endif
	/*  */
EXIT:




	LOG_DBG("- X. Ret: %d. UserCount: %d.", Ret, DPEInfo.UserCount);
	return Ret;

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_release(struct inode *pInode, struct file *pFile)
{
	DPE_USER_INFO_STRUCT *pUserInfo;
	/*MUINT32 Reg;*/

	LOG_DBG("- E. UserCount: %d.", DPEInfo.UserCount);

	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo = (DPE_USER_INFO_STRUCT *) pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	/*  */
	spin_lock(&(DPEInfo.SpinLockDPERef));
	DPEInfo.UserCount--;

	if (DPEInfo.UserCount > 0) {
		spin_unlock(&(DPEInfo.SpinLockDPERef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			DPEInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else
		spin_unlock(&(DPEInfo.SpinLockDPERef));
	/*  */
	LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), last user",
		DPEInfo.UserCount, current->comm, current->pid, current->tgid);


	/* Disable clock. */
	DPE_EnableClock(MFALSE);
	LOG_DBG("DPE release g_u4EnableClockCount: %d", g_u4EnableClockCount);

	/*  */
EXIT:


	LOG_DBG("- X. UserCount: %d.", DPEInfo.UserCount);
	return 0;
}


/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	long length = 0;
	MUINT32 pfn = 0x0;

	length = pVma->vm_end - pVma->vm_start;
	/*  */
	pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
	pfn = pVma->vm_pgoff << PAGE_SHIFT;


	LOG_INF("DPE_mmap: pVma->vm_pgoff(0x%lx)", pVma->vm_pgoff);
	LOG_INF("DPE_mmap: pfn(0x%x),phy(0x%lx)", pfn, pVma->vm_pgoff << PAGE_SHIFT);
	LOG_INF("pVmapVma->vm_start(0x%lx)", pVma->vm_start);
	LOG_INF("pVma->vm_end(0x%lx),length(0x%lx)", pVma->vm_end, length);

	switch (pfn) {
	case DPE_BASE_HW:
		if (length > DPE_REG_RANGE) {
			LOG_ERR("mmap range error :module:0x%x length(0x%lx),DPE_REG_RANGE(0x%x)!",
				pfn, length, DPE_REG_RANGE);
			return -EAGAIN;
		}
		break;
	default:
		LOG_ERR("Illegal starting HW addr for mmap!");
		return -EAGAIN;
	}
	if (remap_pfn_range
	    (pVma, pVma->vm_start, pVma->vm_pgoff, pVma->vm_end - pVma->vm_start,
	     pVma->vm_page_prot)) {
		return -EAGAIN;
	}
	/*  */
	return 0;
}

/*******************************************************************************
*
********************************************************************************/

static dev_t DPEDevNo;
static struct cdev *pDPECharDrv;
static struct class *pDPEClass;

static const struct file_operations DPEFileOper = {
	.owner = THIS_MODULE,
	.open = DPE_open,
	.release = DPE_release,
	/* .flush   = mt_DPE_flush, */
	.mmap = DPE_mmap,
	.unlocked_ioctl = DPE_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = DPE_ioctl_compat,
#endif
};

/*******************************************************************************
*
********************************************************************************/
static inline void DPE_UnregCharDev(void)
{
	LOG_DBG("- E.");
	/*  */
	/* Release char driver */
	if (pDPECharDrv != NULL) {
		cdev_del(pDPECharDrv);
		pDPECharDrv = NULL;
	}
	/*  */
	unregister_chrdev_region(DPEDevNo, 1);
}

/*******************************************************************************
*
********************************************************************************/
static inline MINT32 DPE_RegCharDev(void)
{
	MINT32 Ret = 0;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = alloc_chrdev_region(&DPEDevNo, 0, 1, DPE_DEV_NAME);
	if (Ret < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d", Ret);
		return Ret;
	}
	/* Allocate driver */
	pDPECharDrv = cdev_alloc();
	if (pDPECharDrv == NULL) {
		LOG_ERR("cdev_alloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/* Attatch file operation. */
	cdev_init(pDPECharDrv, &DPEFileOper);
	/*  */
	pDPECharDrv->owner = THIS_MODULE;
	/* Add to system */
	Ret = cdev_add(pDPECharDrv, DPEDevNo, 1);
	if (Ret < 0) {
		LOG_ERR("Attatch file operation failed, %d", Ret);
		goto EXIT;
	}
	/*  */
EXIT:
	if (Ret < 0)
		DPE_UnregCharDev();

	/*  */

	LOG_DBG("- X.");
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_probe(struct platform_device *pDev)
{
	MINT32 Ret = 0;
	/*struct resource *pRes = NULL;*/
	MINT32 i = 0;
	MUINT8 n;
	MUINT32 irq_info[3];	/* Record interrupts info from device tree */
	struct device *dev = NULL;
	struct DPE_device *_dpedev = NULL;

#ifdef CONFIG_OF
	struct DPE_device *DPE_dev;
#endif

	LOG_INF("- E. DPE driver probe.");

	/* Check platform_device parameters */
#ifdef CONFIG_OF

	if (pDev == NULL) {
		dev_err(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}

	nr_DPE_devs += 1;
	_dpedev = krealloc(DPE_devs, sizeof(struct DPE_device) * nr_DPE_devs, GFP_KERNEL);
	if (!_dpedev) {
		dev_err(&pDev->dev, "Unable to allocate DPE_devs\n");
		return -ENOMEM;
	}
	DPE_devs = _dpedev;

	DPE_dev = &(DPE_devs[nr_DPE_devs - 1]);
	DPE_dev->dev = &pDev->dev;

	/* iomap registers */
	DPE_dev->regs = of_iomap(pDev->dev.of_node, 0);
	/* gISPSYS_Reg[nr_DPE_devs - 1] = DPE_dev->regs; */

	if (!DPE_dev->regs) {
		dev_err(&pDev->dev,
			"Unable to ioremap registers, of_iomap fail, nr_DPE_devs=%d, devnode(%s).\n",
			nr_DPE_devs, pDev->dev.of_node->name);
		return -ENOMEM;
	}

	LOG_INF("nr_DPE_devs=%d, devnode(%s), map_addr=0x%lx\n", nr_DPE_devs,
		pDev->dev.of_node->name, (unsigned long)DPE_dev->regs);

	/* get IRQ ID and request IRQ */
	DPE_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);

	if (DPE_dev->irq > 0) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array
		    (pDev->dev.of_node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
			dev_err(&pDev->dev, "get irq flags from DTS fail!!\n");
			return -ENODEV;
		}

		for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++) {
			if (strcmp(pDev->dev.of_node->name, DPE_IRQ_CB_TBL[i].device_name) == 0) {
				Ret =
				    request_irq(DPE_dev->irq,
						(irq_handler_t) DPE_IRQ_CB_TBL[i].isr_fp,
						irq_info[2],
						(const char *)DPE_IRQ_CB_TBL[i].device_name, NULL);
				if (Ret) {
					dev_err(&pDev->dev,
						"Unable to request IRQ, request_irq fail, nr_DPE_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
						nr_DPE_devs, pDev->dev.of_node->name, DPE_dev->irq,
						DPE_IRQ_CB_TBL[i].device_name);
					return Ret;
				}

				LOG_INF("nr_DPE_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
					nr_DPE_devs, pDev->dev.of_node->name, DPE_dev->irq,
					DPE_IRQ_CB_TBL[i].device_name);
				break;
			}
		}

		if (i >= DPE_IRQ_TYPE_AMOUNT) {
			LOG_INF("No corresponding ISR!!: nr_DPE_devs=%d, devnode(%s), irq=%d\n",
				nr_DPE_devs, pDev->dev.of_node->name, DPE_dev->irq);
		}


	} else {
		LOG_INF("No IRQ!!: nr_DPE_devs=%d, devnode(%s), irq=%d\n", nr_DPE_devs,
			pDev->dev.of_node->name, DPE_dev->irq);
	}


#endif

	/* Only register char driver in the 1st time */
	if (nr_DPE_devs == 1) {

		/* Register char driver */
		Ret = DPE_RegCharDev();
		if (Ret) {
			dev_err(&pDev->dev, "register char failed");
			return Ret;
		}
#ifndef __DPE_EP_NO_CLKMGR__
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
		    /*CCF: Grab clock pointer (struct clk*) */
		dpe_clk.CG_IMGSYS_DPE = devm_clk_get(&pDev->dev, "DPE_CLK_IMGSYS_DPE");

		if (IS_ERR(dpe_clk.CG_IMGSYS_DPE)) {
			LOG_ERR("cannot get CG_IMGSYS_DPE clock\n");
			return PTR_ERR(dpe_clk.CG_IMGSYS_DPE);
		}

#endif				/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
#endif

		/* Create class register */
		pDPEClass = class_create(THIS_MODULE, "DPEdrv");
		if (IS_ERR(pDPEClass)) {
			Ret = PTR_ERR(pDPEClass);
			LOG_ERR("Unable to create class, err = %d", Ret);
			goto EXIT;
		}

		dev = device_create(pDPEClass, NULL, DPEDevNo, NULL, DPE_DEV_NAME);

		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_err(&pDev->dev, "Failed to create device: /dev/%s, err = %d",
				DPE_DEV_NAME, Ret);
			goto EXIT;
		}

		/* Init spinlocks */
		spin_lock_init(&(DPEInfo.SpinLockDPERef));
		spin_lock_init(&(DPEInfo.SpinLockDPE));
		for (n = 0; n < DPE_IRQ_TYPE_AMOUNT; n++)
			spin_lock_init(&(DPEInfo.SpinLockIrq[n]));

		/*  */
		init_waitqueue_head(&DPEInfo.WaitQueueHead);
		INIT_WORK(&DPEInfo.ScheduleDveWork, DPE_ScheduleDveWork);
		INIT_WORK(&DPEInfo.ScheduleWmfeWork, DPE_ScheduleWmfeWork);

		wake_lock_init(&DPE_wake_lock, WAKE_LOCK_SUSPEND, "dpe_lock_wakelock");

		for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(DPE_tasklet[i].pDPE_tkt, DPE_tasklet[i].tkt_cb, 0);

		/* Init DPEInfo */
		spin_lock(&(DPEInfo.SpinLockDPERef));
		DPEInfo.UserCount = 0;
		spin_unlock(&(DPEInfo.SpinLockDPERef));
		/*  */
		DPEInfo.IrqInfo.Mask[DPE_IRQ_TYPE_INT_DPE_ST] = INT_ST_MASK_DPE;

	}

EXIT:
	if (Ret < 0)
		DPE_UnregCharDev();


	LOG_INF("- X. DPE driver probe.");

	return Ret;
}

/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static MINT32 DPE_remove(struct platform_device *pDev)
{
	/*struct resource *pRes;*/
	MINT32 IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	/* unregister char driver. */
	DPE_UnregCharDev();

	/* Release IRQ */
	disable_irq(DPEInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* kill tasklet */
	for (i = 0; i < DPE_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(DPE_tasklet[i].pDPE_tkt);
#if 0
	/* free all registered irq(child nodes) */
	DPE_UnRegister_AllregIrq();
	/* free father nodes of irq user list */
	struct my_list_head *head;
	struct my_list_head *father;

	head = ((struct my_list_head *)(&SupIrqUserListHead.list));
	while (1) {
		father = head;
		if (father->nextirq != father) {
			father = father->nextirq;
			REG_IRQ_NODE *accessNode;

			typeof(((REG_IRQ_NODE *) 0)->list) * __mptr = (father);
			accessNode =
			    ((REG_IRQ_NODE *) ((char *)__mptr - offsetof(REG_IRQ_NODE, list)));
			LOG_INF("free father,reg_T(%d)\n", accessNode->reg_T);
			if (father->nextirq != father) {
				head->nextirq = father->nextirq;
				father->nextirq = father;
			} else {	/* last father node */
				head->nextirq = head;
				LOG_INF("break\n");
				break;
			}
			kfree(accessNode);
		}
	}
#endif
	/*  */
	device_destroy(pDPEClass, DPEDevNo);
	/*  */
	class_destroy(pDPEClass);
	pDPEClass = NULL;
	/*  */
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 bPass1_On_In_Resume_TG1;

static MINT32 DPE_suspend(struct platform_device *pDev, pm_message_t Mesg)
{
	/*MINT32 ret = 0;*/

	LOG_DBG("bPass1_On_In_Resume_TG1(%d)\n", bPass1_On_In_Resume_TG1);

	bPass1_On_In_Resume_TG1 = 0;


	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 DPE_resume(struct platform_device *pDev)
{
	LOG_DBG("bPass1_On_In_Resume_TG1(%d).\n", bPass1_On_In_Resume_TG1);

	return 0;
}


/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int DPE_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return DPE_suspend(pdev, PMSG_SUSPEND);
}

int DPE_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return DPE_resume(pdev);
}
#ifndef CONFIG_OF
/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
#endif
int DPE_pm_restore_noirq(struct device *device)
{
	pr_debug("calling %s()\n", __func__);
#ifndef CONFIG_OF
	mt_irq_set_sens(DPE_IRQ_BIT_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(DPE_IRQ_BIT_ID, MT_POLARITY_LOW);
#endif
	return 0;

}

/*---------------------------------------------------------------------------*/
#else				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#define DPE_pm_suspend NULL
#define DPE_pm_resume  NULL
#define DPE_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_OF
/*
 * Note!!! The order and member of .compatible must be the same with that in
 *  "DPE_DEV_NODE_ENUM" in camera_DPE.h
 */
static const struct of_device_id DPE_of_ids[] = {
	{.compatible = "mediatek,dpe",},
	{}
};
#endif

const struct dev_pm_ops DPE_pm_ops = {
	.suspend = DPE_pm_suspend,
	.resume = DPE_pm_resume,
	.freeze = DPE_pm_suspend,
	.thaw = DPE_pm_resume,
	.poweroff = DPE_pm_suspend,
	.restore = DPE_pm_resume,
	.restore_noirq = DPE_pm_restore_noirq,
};


/*******************************************************************************
*
********************************************************************************/
static struct platform_driver DPEDriver = {
	.probe = DPE_probe,
	.remove = DPE_remove,
	.suspend = DPE_suspend,
	.resume = DPE_resume,
	.driver = {
		   .name = DPE_DEV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = DPE_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &DPE_pm_ops,
#endif
		   }
};


static int dpe_dump_read(struct seq_file *m, void *v)
{
	int i, j;

	seq_puts(m, "\n============ dpe dump register============\n");
	seq_puts(m, "DVE Config Info\n");

	for (i = 0x2C; i < 0x8C; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	seq_puts(m, "DVE Debug Info\n");
	for (i = 0x120; i < 0x148; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}

	seq_puts(m, "WMFE Config Info\n");
	for (i = 0x230; i < 0x2D8; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}
	seq_puts(m, "WMFE Debug Info\n");
	for (i = 0x2F4; i < 0x30C; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}

	seq_puts(m, "\n");
	seq_printf(m, "Dpe Clock Count:%d\n", g_u4EnableClockCount);

	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_DMA_DBG_HW),
		   (unsigned int)DPE_RD32(DPE_DMA_DBG_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_DMA_REQ_STATUS_HW),
		   (unsigned int)DPE_RD32(DPE_DMA_REQ_STATUS_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_DMA_RDY_STATUS_HW),
		   (unsigned int)DPE_RD32(DPE_DMA_RDY_STATUS_REG));

	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DPE_DMA_RDY_STATUS_HW),
		   (unsigned int)DPE_RD32(DPE_DMA_RDY_STATUS_REG));


	seq_printf(m, "DVE:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		   g_DVE_RequestRing.HWProcessIdx, g_DVE_RequestRing.WriteIdx,
		   g_DVE_RequestRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		seq_printf(m,
			   "DVE:RequestState:%d, processID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, FrameRDIdx:%d\n",
			   g_DVE_RequestRing.DVEReq_Struct[i].RequestState,
			   g_DVE_RequestRing.DVEReq_Struct[i].processID,
			   g_DVE_RequestRing.DVEReq_Struct[i].callerID,
			   g_DVE_RequestRing.DVEReq_Struct[i].enqueReqNum,
			   g_DVE_RequestRing.DVEReq_Struct[i].FrameWRIdx,
			   g_DVE_RequestRing.DVEReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_;) {
			seq_printf(m,
				   "DVE:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
				   j, g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j]
				   , j + 1,
				   g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j + 1], j + 2,
				   g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j + 2]
				   , j + 3,
				   g_DVE_RequestRing.DVEReq_Struct[i].DveFrameStatus[j + 3]);
			j = j + 4;
		}
	}


	seq_printf(m, "WMFE:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		   g_WMFE_ReqRing.HWProcessIdx, g_WMFE_ReqRing.WriteIdx,
		   g_WMFE_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_DPE_REQUEST_RING_SIZE_; i++) {
		seq_printf(m,
			   "WMFE:RequestState:%d, processID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, FrameRDIdx:%d\n",
			   g_WMFE_ReqRing.WMFEReq_Struct[i].RequestState,
			   g_WMFE_ReqRing.WMFEReq_Struct[i].processID,
			   g_WMFE_ReqRing.WMFEReq_Struct[i].callerID,
			   g_WMFE_ReqRing.WMFEReq_Struct[i].enqueReqNum,
			   g_WMFE_ReqRing.WMFEReq_Struct[i].FrameWRIdx,
			   g_WMFE_ReqRing.WMFEReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_DPE_FRAME_REQUEST_;) {
			seq_printf(m,
				   "WMFE:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
				   j, g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j]
				   , j + 1,
				   g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j + 1],
				   j + 2,
				   g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j + 2]
				   , j + 3,
				   g_WMFE_ReqRing.WMFEReq_Struct[i].WmfeFrameStatus[j + 3]);
			j = j + 4;
		}
	}

	seq_puts(m, "\n============ dpe dump debug ============\n");

	return 0;
}


static int proc_dpe_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, dpe_dump_read, NULL);
}

static const struct file_operations dpe_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_dpe_dump_open,
	.read = seq_read,
};


static int dpe_reg_read(struct seq_file *m, void *v)
{
	unsigned int i;

	seq_puts(m, "======== read dpe register ========\n");

	for (i = 0x1C; i <= 0x308; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(DPE_BASE_HW + i),
			   (unsigned int)DPE_RD32(ISP_DPE_BASE + i));
	}

	seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(DPE_BASE_HW + 0x7F4),
		   (unsigned int)DPE_RD32(DPE_DMA_DBG_REG));
	seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(DPE_BASE_HW + 0x7F8),
		   (unsigned int)DPE_RD32(DPE_DMA_REQ_STATUS_REG));
	seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(DPE_BASE_HW + 0x7FC),
		   (unsigned int)DPE_RD32(DPE_DMA_RDY_STATUS_REG));

	return 0;
}

/*static int dpe_reg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)*/

static ssize_t dpe_reg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	/*char *pEnd;*/
	char addrSzBuf[24];
	char valSzBuf[24];
	char *pszTmp;
	int addr = 0, val = 0;
	long int tempval;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%23s %23s", addrSzBuf, valSzBuf) == 2) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			if (kstrtol(addrSzBuf, 10, (long int *)&tempval) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s", addrSzBuf);
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("scan hexadecimal addr is wrong !!:%s", addrSzBuf);
			} else {
				LOG_INF("DPE Write Addr Error!!:%s", addrSzBuf);
			}
		}

		pszTmp = strstr(valSzBuf, "0x");
		if (pszTmp == NULL) {
			if (kstrtol(valSzBuf, 10, (long int *)&tempval) != 0)
				LOG_ERR("scan decimal value is wrong !!:%s", valSzBuf);
		} else {
			if (strlen(valSzBuf) > 2) {
				if (sscanf(valSzBuf + 2, "%x", &val) != 1)
					LOG_ERR("scan hexadecimal value is wrong !!:%s", valSzBuf);
			} else {
				LOG_INF("DPE Write Value Error!!:%s\n", valSzBuf);
			}
		}

		if ((addr >= DPE_BASE_HW) && (addr <= DPE_DMA_RDY_STATUS_HW)) {
			LOG_INF("Write Request - addr:0x%x, value:0x%x\n", addr, val);
			DPE_WR32((ISP_DPE_BASE + (addr - DPE_BASE_HW)), val);
		} else {
			LOG_INF
			    ("Write-Address Range exceeds the size of hw dpe!! addr:0x%x, value:0x%x\n",
			     addr, val);
		}

	} else if (sscanf(desc, "%23s", addrSzBuf) == 1) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			if (kstrtol(addrSzBuf, 10, (long int *)&tempval) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s", addrSzBuf);
			else
				addr = tempval;
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("scan hexadecimal addr is wrong !!:%s", addrSzBuf);
			} else {
				LOG_INF("DPE Read Addr Error!!:%s", addrSzBuf);
			}
		}

		if ((addr >= DPE_BASE_HW) && (addr <= DPE_DMA_RDY_STATUS_HW)) {
			val = DPE_RD32((ISP_DPE_BASE + (addr - DPE_BASE_HW)));
			LOG_INF("Read Request - addr:0x%x,value:0x%x\n", addr, val);
		} else {
			LOG_INF
			    ("Read-Address Range exceeds the size of hw dpe!! addr:0x%x, value:0x%x\n",
			     addr, val);
		}

	}


	return count;
}

static int proc_dpe_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, dpe_reg_read, NULL);
}

static const struct file_operations dpe_reg_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_dpe_reg_open,
	.read = seq_read,
	.write = dpe_reg_write,
};


/*******************************************************************************
*
********************************************************************************/

int32_t DPE_ClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("DPE_ClockOnCallback"); */
	/* LOG_DBG("+CmdqEn:%d", g_u4EnableClockCount); */
	/* DPE_EnableClock(MTRUE); */

	return 0;
}

int32_t DPE_DumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("DPE_DumpCallback");

	DPE_DumpReg();

	return 0;
}

int32_t DPE_ResetCallback(uint64_t engineFlag)
{
	LOG_DBG("DPE_ResetCallback");
	DPE_Reset();

	return 0;
}

int32_t DPE_ClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("DPE_ClockOffCallback"); */
	/* DPE_EnableClock(MFALSE); */
	/* LOG_DBG("-CmdqEn:%d", g_u4EnableClockCount); */
	return 0;
}


static MINT32 __init DPE_Init(void)
{
	MINT32 Ret = 0, j;
	void *tmp;
	/* FIX-ME: linux-3.10 procfs API changed */
	/* use proc_create */
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *isp_dpe_dir;


	int i;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = platform_driver_register(&DPEDriver);
	if (Ret < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}

#if 0
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,DPE");
	if (!node) {
		LOG_ERR("find mediatek,DPE node failed!!!\n");
		return -ENODEV;
	}
	ISP_DPE_BASE = of_iomap(node, 0);
	if (!ISP_DPE_BASE) {
		LOG_ERR("unable to map ISP_DPE_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_DPE_BASE: %lx\n", ISP_DPE_BASE);
#endif

	isp_dpe_dir = proc_mkdir("dpe", NULL);
	if (!isp_dpe_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/dpe\n", __func__);
		return 0;
	}

	/* proc_entry = proc_create("pll_test", S_IRUGO | S_IWUSR, isp_dpe_dir, &pll_test_proc_fops); */

	proc_entry = proc_create("dpe_dump", S_IRUGO, isp_dpe_dir, &dpe_dump_proc_fops);

	proc_entry = proc_create("dpe_reg", S_IRUGO | S_IWUSR, isp_dpe_dir, &dpe_reg_proc_fops);


	/* isr log */
	if (PAGE_SIZE <
	    ((DPE_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN * ((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) *
	     LOG_PPNUM)) {
		i = 0;
		while (i <
		       ((DPE_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
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
		for (j = 0; j < DPE_IRQ_TYPE_AMOUNT; j++) {
			gSvLog[j]._str[i][_LOG_DBG] = (char *)tmp;
			/* tmp = (void*) ((unsigned int)tmp + (NORMAL_STR_LEN*DBG_PAGE)); */
			tmp = (void *)((char *)tmp + (NORMAL_STR_LEN * DBG_PAGE));
			gSvLog[j]._str[i][_LOG_INF] = (char *)tmp;
			/* tmp = (void*) ((unsigned int)tmp + (NORMAL_STR_LEN*INF_PAGE)); */
			tmp = (void *)((char *)tmp + (NORMAL_STR_LEN * INF_PAGE));
			gSvLog[j]._str[i][_LOG_ERR] = (char *)tmp;
			/* tmp = (void*) ((unsigned int)tmp + (NORMAL_STR_LEN*ERR_PAGE)); */
			tmp = (void *)((char *)tmp + (NORMAL_STR_LEN * ERR_PAGE));
		}
		/* tmp = (void*) ((unsigned int)tmp + NORMAL_STR_LEN); //log buffer ,in case of overflow */
		tmp = (void *)((char *)tmp + NORMAL_STR_LEN);	/* log buffer ,in case of overflow */
	}


#if 1
	/* Cmdq */
	/* Register DPE callback */
	LOG_DBG("register dpe callback for CMDQ");
	cmdqCoreRegisterCB(CMDQ_GROUP_DPE,
			   DPE_ClockOnCallback,
			   DPE_DumpCallback, DPE_ResetCallback, DPE_ClockOffCallback);
#endif

	LOG_DBG("- X. Ret: %d.", Ret);
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static void __exit DPE_Exit(void)
{
	/*int i;*/

	LOG_DBG("- E.");
	/*  */
	platform_driver_unregister(&DPEDriver);
	/*  */
#if 1
	/* Cmdq */
	/* Unregister DPE callback */
	cmdqCoreRegisterCB(CMDQ_GROUP_DPE, NULL, NULL, NULL, NULL);
#endif

	kfree(pLog_kmalloc);

	/*  */
}


/*******************************************************************************
*
********************************************************************************/
static void DPE_ScheduleDveWork(struct work_struct *data)
{
	if (DPE_DBG_DBGLOG & DPEInfo.DebugMask)
		LOG_DBG("- E.");

}

/*******************************************************************************
*
********************************************************************************/
static void DPE_ScheduleWmfeWork(struct work_struct *data)
{
	if (DPE_DBG_DBGLOG & DPEInfo.DebugMask)
		LOG_DBG("- E.");
}


static irqreturn_t ISP_Irq_DPE(MINT32 Irq, void *DeviceId)
{
	MUINT32 DpeIntStatus;
	MUINT32 DveStatus;
	MUINT32 WmfeStatus;
	MUINT32 DpeDveSta0;
	MBOOL bResulst = MFALSE;
	pid_t ProcessID;

	DpeIntStatus = DPE_RD32(DPE_INT_STATUS_REG);	/* DPE_INT_STATUS */
	DveStatus = DPE_RD32(DPE_DVE_INT_STATUS_REG);	/* DVE Status */
	WmfeStatus = DPE_RD32(DPE_WMFE_INT_STATUS_REG);	/* WMFE Status */
	DpeDveSta0 = DPE_RD32(DPE_DVE_STA_REG);	/* WMFE Status */
	spin_lock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]));
	if (DVE_INT_ST == (DVE_INT_ST & DveStatus)) {
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("dpe_dve_irq");
#endif
		/* Update the frame status. */
#ifndef DPE_USE_GCE
		DPE_WR32(DPE_DVE_START_REG, 0);
#endif
		bResulst = UpdateDVE(DpeDveSta0, &ProcessID);
		/* Config the Next frame */
		if (bResulst == MTRUE) {
			schedule_work(&DPEInfo.ScheduleDveWork);
#ifdef DPE_USE_GCE
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DPE_ST] |= DPE_DVE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[DPE_PROCESS_ID_DVE] = ProcessID;
			DPEInfo.IrqInfo.DveIrqCnt++;
			DPEInfo.ProcessID[DPEInfo.WriteReqIdx] = ProcessID;
			DPEInfo.WriteReqIdx =
			    (DPEInfo.WriteReqIdx + 1) % _SUPPORT_MAX_DPE_FRAME_REQUEST_;
#ifdef DPE_MULTIPROCESS_TIMEING_ISSUE
			/* check the write value is equal to read value ? */
			/* actually, it doesn't happen!! */
			if (DPEInfo.WriteReqIdx == DPEInfo.ReadReqIdx) {
				IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR,
					       "ISP_Irq_DPE Err!!, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
					       DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);
			}
#endif

#else
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DPE_ST] |= DPE_DVE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[DPE_PROCESS_ID_DVE] = ProcessID;
#endif
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
	}

	if (WMFE_INT_ST == (WMFE_INT_ST & WmfeStatus)) {
		/* Update the frame status. */
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("dpe_wmfe_irq");
#endif

#ifndef DPE_USE_GCE
		DPE_WR32(DPE_WMFE_START_REG, 0);
#endif
		bResulst = UpdateWMFE(&ProcessID);
		if (bResulst == MTRUE) {
			schedule_work(&DPEInfo.ScheduleWmfeWork);
#ifdef DPE_USE_GCE
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DPE_ST] |= DPE_WMFE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[DPE_PROCESS_ID_WMFE] = ProcessID;
			DPEInfo.IrqInfo.WmfeIrqCnt++;
			DPEInfo.ProcessID[DPEInfo.WriteReqIdx] = ProcessID;
			DPEInfo.WriteReqIdx =
			    (DPEInfo.WriteReqIdx + 1) % _SUPPORT_MAX_DPE_FRAME_REQUEST_;
#ifdef DPE_MULTIPROCESS_TIMEING_ISSUE
			/* check the write value is equal to read value ? */
			/* actually, it doesn't happen!! */
			if (DPEInfo.WriteReqIdx == DPEInfo.ReadReqIdx) {
				IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR,
					       "ISP_Irq_DPE Err!!, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
					       DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);
			}
#endif

#else
			DPEInfo.IrqInfo.Status[DPE_IRQ_TYPE_INT_DPE_ST] |= DPE_WMFE_INT_ST;
			DPEInfo.IrqInfo.ProcessID[DPE_PROCESS_ID_WMFE] = ProcessID;
#endif
		}
#ifdef __DPE_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}
	spin_unlock(&(DPEInfo.SpinLockIrq[DPE_IRQ_TYPE_INT_DPE_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&DPEInfo.WaitQueueHead);

	/* dump log, use tasklet */
	IRQ_LOG_KEEPER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_INF,
		       "ISP_Irq_DPE:%d, reg 0x%x : 0x%x, bResulst:%d, DveHWSta:0x%x, WmfeHWSta:0x%x, DpeDveSta0:0x%x, DveIrqCnt:0x%x, WmfeIrqCnt:0x%x, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
		       Irq, DPE_INT_STATUS_HW, DpeIntStatus, bResulst, DveStatus, WmfeStatus,
		       DpeDveSta0, DPEInfo.IrqInfo.DveIrqCnt, DPEInfo.IrqInfo.WmfeIrqCnt,
		       DPEInfo.WriteReqIdx, DPEInfo.ReadReqIdx);

	if (DpeIntStatus & DPE_INT_ST)
		tasklet_schedule(DPE_tasklet[DPE_IRQ_TYPE_INT_DPE_ST].pDPE_tkt);

	return IRQ_HANDLED;
}

static void ISP_TaskletFunc_DPE(unsigned long data)
{
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(DPE_IRQ_TYPE_INT_DPE_ST, m_CurrentPPB, _LOG_ERR);

}


/*******************************************************************************
*
********************************************************************************/
module_init(DPE_Init);
module_exit(DPE_Exit);
MODULE_DESCRIPTION("Camera DPE driver");
MODULE_AUTHOR("ME8");
MODULE_LICENSE("GPL");
