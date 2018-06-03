/*
 * Copyright(C)2015 MediaTek Inc.
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

/*#include <linux/xlog.h>		 For xlog_printk(). */
/*  */
/*#include <mach/hardware.h>*/
/* #include <mach/mt6593_pll.h> */
#include "inc/camera_gepf.h"
/* #include <mach/irqs.h> */
/* #include <mach/mt_reg_base.h> */
/* #if defined(CONFIG_MTK_LEGACY) */
/* #include <mach/mt_clkmgr.h> */	/* For clock mgr APIS. enable_clock()/disable_clock(). */
/* #endif */
#include <mt-plat/sync_write.h>	/* For mt65xx_reg_sync_writel(). */
/* #include <mach/mt_spm_idle.h>	 For spm_enable_sodi()/spm_disable_sodi(). */

#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <m4u.h>
#include <cmdq_core.h>
#include <cmdq_record.h>
#include <smi_public.h>

/* Measure the kernel performance
* #define __GEPF_KERNEL_PERFORMANCE_MEASURE__
*/
#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
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

/* GEPF Command Queue */
/* #include "../../cmdq/mt6797/cmdq_record.h" */
/* #include "../../cmdq/mt6797/cmdq_core.h" */

/* CCF */
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#include <linux/clk.h>
struct GEPF_CLK_STRUCT {
	struct clk *CG_SCP_SYS_MM0;
	struct clk *CG_MM_SMI_COMMON;
	struct clk *CG_MM_SMI_COMMON_2X;
	struct clk *CG_MM_SMI_COMMON_GALS_M0_2X;
	struct clk *CG_MM_SMI_COMMON_GALS_M1_2X;
	struct clk *CG_MM_SMI_COMMON_UPSZ0;
	struct clk *CG_MM_SMI_COMMON_UPSZ1;
	struct clk *CG_MM_SMI_COMMON_FIFO0;
	struct clk *CG_MM_SMI_COMMON_FIFO1;
	struct clk *CG_MM_LARB5;
	struct clk *CG_SCP_SYS_ISP;
	struct clk *CG_IMGSYS_LARB;
	struct clk *CG_IMGSYS_GEPF;
};
struct GEPF_CLK_STRUCT gepf_clk;
#endif				/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
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

#define GEPF_DEV_NAME                "camera-gepf"
/*#define EP_NO_CLKMGR*/
#define BYPASS_REG         (0)
/* #define GEPF_WAITIRQ_LOG  */
#define GEPF_USE_GCE
#define GEPF_DEBUG_USE
/* #define GEPF_MULTIPROCESS_TIMEING_ISSUE  */
/*I can' test the situation in FPGA, because the velocity of FPGA is so slow. */
#define MyTag "[GEPF]"
#define IRQTag "KEEPER"

#define LOG_VRB(format,	args...)    pr_debug(MyTag format, ##args)

#ifdef GEPF_DEBUG_USE
#define LOG_DBG(format, args...)    pr_err(MyTag format, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...)    pr_err(MyTag format,  ##args)
#define LOG_NOTICE(format, args...) pr_notice(MyTag format,  ##args)
#define LOG_WRN(format, args...)    pr_warn(MyTag format,  ##args)
#define LOG_ERR(format, args...)    pr_err(MyTag format,  ##args)
#define LOG_AST(format, args...)    pr_alert(MyTag format, ##args)


/*******************************************************************************
*
********************************************************************************/
/* #define GEPF_WR32(addr, data)    iowrite32(data, addr) // For other projects. */
#define GEPF_WR32(addr, data)    mt_reg_sync_writel(data, addr)	/* For 89 Only.   // NEED_TUNING_BY_PROJECT */
#define GEPF_RD32(addr)          ioread32(addr)
#define GEPF_SET_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) |= (MUINT32)(1 << (bit)))
#define GEPF_CLR_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) &= ~((MUINT32)(1 << (bit))))
/*******************************************************************************
*
********************************************************************************/
/* dynamic log level */
#define GEPF_DBG_DBGLOG              (0x00000001)
#define GEPF_DBG_INFLOG              (0x00000002)
#define GEPF_DBG_INT                 (0x00000004)
#define GEPF_DBG_READ_REG            (0x00000008)
#define GEPF_DBG_WRITE_REG           (0x00000010)
#define GEPF_DBG_TASKLET             (0x00000020)


/* ///////////////////////////////////////////////////////////////// */

/*******************************************************************************
*
********************************************************************************/

/*
*    CAM interrupt status
*/
/* normal siganl : happens to be the same bit as register bit*/
/* #define GEPF_INT_ST           (1<<0) */


/*
*    IRQ signal mask
*/

#define INT_ST_MASK_GEPF     ( \
			GEPF_INT_ST)

#define CMDQ_REG_MASK 0xffffffff
#define GEPF_START      0x2

#define GEPF_ENABLE     0x1

#define GEPF_IS_BUSY    0x2


/* static irqreturn_t GEPF_Irq_CAM_A(MINT32  Irq,void *DeviceId); */
static irqreturn_t ISP_Irq_GEPF(MINT32 Irq, void *DeviceId);
static MBOOL ConfigGEPF(void);
static MINT32 ConfigGEPFHW(GEPF_Config *pGepfConfig);
static void GEPF_ScheduleWork(struct work_struct *data);



typedef irqreturn_t(*IRQ_CB) (MINT32, void *);

typedef struct {
	IRQ_CB isr_fp;
	unsigned int int_number;
	char device_name[16];
} ISR_TABLE;

#ifndef CONFIG_OF
const ISR_TABLE GEPF_IRQ_CB_TBL[GEPF_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_GEPF, GEPF_IRQ_BIT_ID, "gepf"},
};

#else
/* int number is got from kernel api */
const ISR_TABLE GEPF_IRQ_CB_TBL[GEPF_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_GEPF, 0, "gepf"},
};

#endif
/* //////////////////////////////////////////////////////////////////////////////////////////// */
/*  */
typedef void (*tasklet_cb) (unsigned long);
typedef struct {
	tasklet_cb tkt_cb;
	struct tasklet_struct *pGEPF_tkt;
} Tasklet_table;

struct tasklet_struct Gepftkt[GEPF_IRQ_TYPE_AMOUNT];

static void ISP_TaskletFunc_GEPF(unsigned long data);

static Tasklet_table GEPF_tasklet[GEPF_IRQ_TYPE_AMOUNT] = {
	{ISP_TaskletFunc_GEPF, &Gepftkt[GEPF_IRQ_TYPE_INT_GEPF_ST]},
};

struct wake_lock GEPF_wake_lock;


static DEFINE_MUTEX(gGepfMutex);
static DEFINE_MUTEX(gGepfDequeMutex);

#ifdef CONFIG_OF

struct GEPF_device {
	void __iomem *regs;
	struct device *dev;
	int irq;
};

static struct GEPF_device *GEPF_devs;
static int nr_GEPF_devs;

/* Get HW modules' base address from device nodes */
#define GEPF_DEV_NODE_IDX 0

/* static unsigned long gISPSYS_Reg[GEPF_IRQ_TYPE_AMOUNT]; */


#define ISP_GEPF_BASE                  (GEPF_devs[GEPF_DEV_NODE_IDX].regs)
/* #define ISP_GEPF_BASE                  (gISPSYS_Reg[GEPF_DEV_NODE_IDX]) */



#else
#define ISP_GEPF_BASE                        (IMGSYS_BASE + 0x2C000)

#endif


static unsigned int g_u4EnableClockCount;

/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32


typedef enum {
	GEPF_FRAME_STATUS_EMPTY,	/* 0 */
	GEPF_FRAME_STATUS_ENQUE,	/* 1 */
	GEPF_FRAME_STATUS_RUNNING,	/* 2 */
	GEPF_FRAME_STATUS_FINISHED,	/* 3 */
	GEPF_FRAME_STATUS_TOTAL
} GEPF_FRAME_STATUS_ENUM;


typedef enum {
	GEPF_REQUEST_STATE_EMPTY,	/* 0 */
	GEPF_REQUEST_STATE_PENDING,	/* 1 */
	GEPF_REQUEST_STATE_RUNNING,	/* 2 */
	GEPF_REQUEST_STATE_FINISHED,	/* 3 */
	GEPF_REQUEST_STATE_TOTAL
} GEPF_REQUEST_STATE_ENUM;


typedef struct {
	GEPF_REQUEST_STATE_ENUM State;
	pid_t processID;	/* caller process ID */
	MUINT32 callerID;	/* caller thread ID */
	MUINT32 enqueReqNum;	/* to judge it belongs to which frame package */
	MINT32 FrameWRIdx;	/* Frame write Index */
	MINT32 FrameRDIdx;	/* Frame read Index */
	GEPF_FRAME_STATUS_ENUM GepfFrameStatus[_SUPPORT_MAX_GEPF_FRAME_REQUEST_];
	GEPF_Config GepfFrameConfig[_SUPPORT_MAX_GEPF_FRAME_REQUEST_];
} GEPF_REQUEST_STRUCT;

typedef struct {
	MINT32 WriteIdx;	/* enque how many request  */
	MINT32 ReadIdx;		/* read which request index */
	MINT32 HWProcessIdx;	/* HWWriteIdx */
	GEPF_REQUEST_STRUCT GEPFReq_Struct[_SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_];
} GEPF_REQUEST_RING_STRUCT;

typedef struct {
	GEPF_Config GepfFrameConfig[_SUPPORT_MAX_GEPF_FRAME_REQUEST_];
} GEPF_CONFIG_STRUCT;

static GEPF_REQUEST_RING_STRUCT g_GEPF_ReqRing;
static GEPF_CONFIG_STRUCT g_GepfEnqueReq_Struct;
static GEPF_CONFIG_STRUCT g_GepfDequeReq_Struct;


/*******************************************************************************
*
********************************************************************************/
typedef struct {
	pid_t Pid;
	pid_t Tid;
} GEPF_USER_INFO_STRUCT;
typedef enum {
	GEPF_PROCESS_ID_NONE,
	GEPF_PROCESS_ID_GEPF,
	GEPF_PROCESS_ID_AMOUNT
} GEPF_PROCESS_ID_ENUM;


/*******************************************************************************
*
********************************************************************************/
typedef struct {
	volatile MUINT32 Status[GEPF_IRQ_TYPE_AMOUNT];
	MINT32 GepfIrqCnt;
	pid_t ProcessID[GEPF_PROCESS_ID_AMOUNT];
	MUINT32 Mask[GEPF_IRQ_TYPE_AMOUNT];
} GEPF_IRQ_INFO_STRUCT;


typedef struct {
	spinlock_t SpinLockGEPFRef;
	spinlock_t SpinLockGEPF;
	spinlock_t SpinLockIrq[GEPF_IRQ_TYPE_AMOUNT];
	wait_queue_head_t WaitQueueHead;
	struct work_struct ScheduleGepfWork;
	MUINT32 UserCount;	/* User Count */
	MUINT32 DebugMask;	/* Debug Mask */
	MINT32 IrqNum;
	GEPF_IRQ_INFO_STRUCT IrqInfo;
	MINT32 WriteReqIdx;
	MINT32 ReadReqIdx;
	pid_t ProcessID[_SUPPORT_MAX_GEPF_FRAME_REQUEST_];
} GEPF_INFO_STRUCT;


static GEPF_INFO_STRUCT GEPFInfo;

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
static SV_LOG_STR gSvLog[GEPF_IRQ_TYPE_AMOUNT];

/*
*    for irq used,keep log until IRQ_LOG_PRINTER being involked,
*    limited:
*    each log must shorter than 512 bytes
*    total log length in each irq/logtype can't over 1024 bytes
*/
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
#else
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...)  xlog_printk(ANDROID_LOG_DEBUG,\
"KEEPER", "[%s] " fmt, __func__, ##__VA_ARGS__)
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

/* GEPF unmapped base address macro for GCE to access */
#define GEPF_DMA_DMA_SOFT_RSTSTAT_HW  (GEPF_BASE_HW)
#define GEPF_DMA_TDRI_BASE_ADDR_HW    (GEPF_BASE_HW + 0x04)
#define GEPF_DMA_TDRI_OFST_ADDR_HW    (GEPF_BASE_HW + 0x08)
#define GEPF_DMA_TDRI_XSIZE_HW        (GEPF_BASE_HW + 0x0C)
#define GEPF_DMA_VERTICAL_FLIP_EN_HW  (GEPF_BASE_HW + 0x10)
#define GEPF_DMA_DMA_SOFT_RESET_HW    (GEPF_BASE_HW + 0x14)
#define GEPF_DMA_LAST_ULTRA_EN_HW     (GEPF_BASE_HW + 0x18)
#define GEPF_DMA_SPECIAL_FUN_EN_HW    (GEPF_BASE_HW + 0x1C)
#define GEPF_DMA_GEPFO_BASE_ADDR_HW   (GEPF_BASE_HW + 0x30)
#define GEPF_DMA_GEPFO_OFST_ADDR_HW   (GEPF_BASE_HW + 0x38)
#define GEPF_DMA_GEPFO_XSIZE_HW       (GEPF_BASE_HW + 0x40)
#define GEPF_DMA_GEPFO_YSIZE_HW       (GEPF_BASE_HW + 0x44)
#define GEPF_DMA_GEPFO_STRIDE_HW      (GEPF_BASE_HW + 0x48)
#define GEPF_DMA_GEPFO_CON_HW         (GEPF_BASE_HW + 0x4C)
#define GEPF_DMA_GEPFO_CON2_HW        (GEPF_BASE_HW + 0x50)
#define GEPF_DMA_GEPFO_CON3_HW        (GEPF_BASE_HW + 0x54)
#define GEPF_DMA_GEPFO_CROP_HW        (GEPF_BASE_HW + 0x58)
#define GEPF_DMA_GEPF2O_BASE_ADDR_HW  (GEPF_BASE_HW + 0x60)
#define GEPF_DMA_GEPF2O_OFST_ADDR_HW  (GEPF_BASE_HW + 0x68)
#define GEPF_DMA_GEPF2O_XSIZE_HW      (GEPF_BASE_HW + 0x70)
#define GEPF_DMA_GEPF2O_YSIZE_HW      (GEPF_BASE_HW + 0x74)
#define GEPF_DMA_GEPF2O_STRIDE_HW     (GEPF_BASE_HW + 0x78)
#define GEPF_DMA_GEPF2O_CON_HW        (GEPF_BASE_HW + 0x7C)
#define GEPF_DMA_GEPF2O_CON2_HW       (GEPF_BASE_HW + 0x80)
#define GEPF_DMA_GEPF2O_CON3_HW       (GEPF_BASE_HW + 0x84)
#define GEPF_DMA_GEPF2O_CROP_HW       (GEPF_BASE_HW + 0x88)
#define GEPF_DMA_GEPFI_BASE_ADDR_HW   (GEPF_BASE_HW + 0x90)
#define GEPF_DMA_GEPFI_OFST_ADDR_HW   (GEPF_BASE_HW + 0x98)
#define GEPF_DMA_GEPFI_XSIZE_HW       (GEPF_BASE_HW + 0xA0)
#define GEPF_DMA_GEPFI_YSIZE_HW       (GEPF_BASE_HW + 0xA4)
#define GEPF_DMA_GEPFI_STRIDE_HW      (GEPF_BASE_HW + 0xA8)
#define GEPF_DMA_GEPFI_CON_HW         (GEPF_BASE_HW + 0xAC)
#define GEPF_DMA_GEPFI_CON2_HW        (GEPF_BASE_HW + 0xB0)
#define GEPF_DMA_GEPFI_CON3_HW        (GEPF_BASE_HW + 0xB4)
#define GEPF_DMA_GEPF2I_BASE_ADDR_HW  (GEPF_BASE_HW + 0xC0)
#define GEPF_DMA_GEPF2I_OFST_ADDR_HW  (GEPF_BASE_HW + 0xC8)
#define GEPF_DMA_GEPF2I_XSIZE_HW      (GEPF_BASE_HW + 0xD0)
#define GEPF_DMA_GEPF2I_YSIZE_HW      (GEPF_BASE_HW + 0xD4)
#define GEPF_DMA_GEPF2I_STRIDE_HW     (GEPF_BASE_HW + 0xD8)
#define GEPF_DMA_GEPF2I_CON_HW        (GEPF_BASE_HW + 0xDC)
#define GEPF_DMA_GEPF2I_CON2_HW       (GEPF_BASE_HW + 0xE0)
#define GEPF_DMA_GEPF2I_CON3_HW       (GEPF_BASE_HW + 0xE4)
#define GEPF_DMA_GEPF3I_BASE_ADDR_HW  (GEPF_BASE_HW + 0xF0)
#define GEPF_DMA_GEPF3I_OFST_ADDR_HW  (GEPF_BASE_HW + 0xF8)
#define GEPF_DMA_GEPF3I_XSIZE_HW      (GEPF_BASE_HW + 0x100)
#define GEPF_DMA_GEPF3I_YSIZE_HW      (GEPF_BASE_HW + 0x104)
#define GEPF_DMA_GEPF3I_STRIDE_HW     (GEPF_BASE_HW + 0x108)
#define GEPF_DMA_GEPF3I_CON_HW        (GEPF_BASE_HW + 0x10C)
#define GEPF_DMA_GEPF3I_CON2_HW       (GEPF_BASE_HW + 0x110)
#define GEPF_DMA_GEPF3I_CON3_HW       (GEPF_BASE_HW + 0x114)
#define GEPF_DMA_GEPF4I_BASE_ADDR_HW  (GEPF_BASE_HW + 0x120)
#define GEPF_DMA_GEPF4I_OFST_ADDR_HW  (GEPF_BASE_HW + 0x128)
#define GEPF_DMA_GEPF4I_XSIZE_HW      (GEPF_BASE_HW + 0x130)
#define GEPF_DMA_GEPF4I_YSIZE_HW      (GEPF_BASE_HW + 0x134)
#define GEPF_DMA_GEPF4I_STRIDE_HW     (GEPF_BASE_HW + 0x138)
#define GEPF_DMA_GEPF4I_CON_HW        (GEPF_BASE_HW + 0x13C)
#define GEPF_DMA_GEPF4I_CON2_HW       (GEPF_BASE_HW + 0x140)
#define GEPF_DMA_GEPF4I_CON3_HW       (GEPF_BASE_HW + 0x144)
#define GEPF_DMA_DMA_ERR_CTRL_HW      (GEPF_BASE_HW + 0x150)
#define GEPF_DMA_GEPFO_ERR_STAT_HW    (GEPF_BASE_HW + 0x154)
#define GEPF_DMA_GEPF2O_ERR_STAT_HW   (GEPF_BASE_HW + 0x158)
#define GEPF_DMA_GEPFI_ERR_STAT_HW    (GEPF_BASE_HW + 0x15C)
#define GEPF_DMA_GEPF2I_ERR_STAT_HW   (GEPF_BASE_HW + 0x160)
#define GEPF_DMA_GEPF3I_ERR_STAT_HW   (GEPF_BASE_HW + 0x164)
#define GEPF_DMA_GEPF4I_ERR_STAT_HW   (GEPF_BASE_HW + 0x168)
#define GEPF_DMA_DMA_DEBUG_ADDR_HW    (GEPF_BASE_HW + 0x16C)
#define GEPF_DMA_DMA_RSV1_HW          (GEPF_BASE_HW + 0x170)
#define GEPF_DMA_DMA_RSV2_HW          (GEPF_BASE_HW + 0x174)
#define GEPF_DMA_DMA_RSV3_HW          (GEPF_BASE_HW + 0x178)
#define GEPF_DMA_DMA_RSV4_HW          (GEPF_BASE_HW + 0x17C)
#define GEPF_DMA_DMA_DEBUG_SEL_HW     (GEPF_BASE_HW + 0x180)
#define GEPF_DMA_DMA_BW_SELF_TEST_HW  (GEPF_BASE_HW + 0x184)

#define GEPF_IRQ_HW                   (GEPF_BASE_HW + 0x400)
#define GEPF_CRT_0_HW                 (GEPF_BASE_HW + 0x404)
#define GEPF_CRT_1_HW                 (GEPF_BASE_HW + 0x408)
#define GEPF_CRT_2_HW                 (GEPF_BASE_HW + 0x48C)
#define GEPF_FOCUS_BASE_ADDR_HW       (GEPF_BASE_HW + 0x40C)
#define GEPF_FOCUS_OFFSET_HW          (GEPF_BASE_HW + 0x410)
#define GEPF_Y_BASE_ADDR_HW           (GEPF_BASE_HW + 0x414)
#define GEPF_YUV_IMG_SIZE_HW          (GEPF_BASE_HW + 0x418)
#define GEPF_UV_BASE_ADDR_HW          (GEPF_BASE_HW + 0x41C)
#define GEPF_DEPTH_BASE_ADDR_HW       (GEPF_BASE_HW + 0x420)
#define GEPF_DEPTH_IMG_SIZE_HW        (GEPF_BASE_HW + 0x428)
#define GEPF_BLUR_BASE_ADDR_HW        (GEPF_BASE_HW + 0x42C)
#define GEPF_YUV_BASE_ADDR_HW         (GEPF_BASE_HW + 0x430)
#define TEMP_CTR_0_HW                 (GEPF_BASE_HW + 0x434)
#define TEMP_YUV_IMG_SIZE_HW          (GEPF_BASE_HW + 0x438)
#define TEMP_DEPTH_IMG_SIZE_HW        (GEPF_BASE_HW + 0x43C)
#define TEMP_PRE_Y_BASE_ADDR_HW       (GEPF_BASE_HW + 0x440)
#define TEMP_Y_BASE_ADDR_HW           (GEPF_BASE_HW + 0x444)
#define TEMP_DEPTH_BASE_ADDR_HW       (GEPF_BASE_HW + 0x448)
#define TEMP_PRE_DEPTH_BASE_ADDR_HW   (GEPF_BASE_HW + 0x44C)
#define BYPASS_DEPTH_BASE_ADDR_HW     (GEPF_BASE_HW + 0x450)
#define BYPASS_DEPTH_WR_BASE_ADDR_HW  (GEPF_BASE_HW + 0x454)
#define BYPASS_BLUR_BASE_ADDR_HW      (GEPF_BASE_HW + 0x458)
#define BYPASS_BLUR_WR_BASE_ADDR_HW   (GEPF_BASE_HW + 0x45C)
#define TEMP_BLUR_BASE_ADDR_HW        (GEPF_BASE_HW + 0x460)
#define TEMP_PRE_BLUR_BASE_ADDR_HW    (GEPF_BASE_HW + 0x464)
#define TEMP_BLUR_WR_BASE_ADDR_HW     (GEPF_BASE_HW + 0x468)
#define TEMP_DEPTH_WR_BASE_ADDR_HW    (GEPF_BASE_HW + 0x46C)
#define GEPF_DEPTH_WR_BASE_ADDR_HW    (GEPF_BASE_HW + 0x470)
#define GEPF_BLUR_WR_BASE_ADDR_HW     (GEPF_BASE_HW + 0x474)
#define GEPF_DMA_CTL_HW               (GEPF_BASE_HW + 0x478)
#define GEPF_DMA_RST_HW               (GEPF_BASE_HW + 0x47C)
#define GEPF_DMA_DCM_HW               (GEPF_BASE_HW + 0x480)
#define GEPF_MODULE_DCM_HW            (GEPF_BASE_HW + 0x484)
#define TEMP_CTR_1_HW                 (GEPF_BASE_HW + 0x488)
#define GEPF_480_Y_ADDR_HW            (GEPF_BASE_HW + 0x490)
#define GEPF_480_UV_ADDR_HW           (GEPF_BASE_HW + 0x494)
#define GEPF_STRIDE_Y_UV_HW           (GEPF_BASE_HW + 0x498)
#define GEPF_STRIDE_DEPTH_HW          (GEPF_BASE_HW + 0x49C)
#define TEMP_STRIDE_DEPTH_HW          (GEPF_BASE_HW + 0x4A0)
#define GEPF_STRIDE_Y_UV_480_HW       (GEPF_BASE_HW + 0x4A4)
#define DMA_DEBUG_DATA_HW             (GEPF_BASE_HW + 0x4A8)
#define DMA_DCM_ST_HW                 (GEPF_BASE_HW + 0x4AC)
#define DMA_RDY_ST_HW                 (GEPF_BASE_HW + 0x4B0)
#define DMA_REQ_ST_HW                 (GEPF_BASE_HW + 0x4B4)
#define GEPF_DEBUG_SEL_HW             (GEPF_BASE_HW + 0x4C0)
#define GEPF_DEBUG_0_HW               (GEPF_BASE_HW + 0x4C4)
#define GEPF_DEBUG_1_HW               (GEPF_BASE_HW + 0x4C8)
#define GEPF_MEM_STALL_CNT_HW         (GEPF_BASE_HW + 0x4CC)

/*SW Access Registers : using mapped base address from DTS*/
#define GEPF_DMA_DMA_SOFT_RSTSTAT_REG  (ISP_GEPF_BASE)
#define GEPF_DMA_TDRI_BASE_ADDR_REG    (ISP_GEPF_BASE + 0x04)
#define GEPF_DMA_TDRI_OFST_ADDR_REG    (ISP_GEPF_BASE + 0x08)
#define GEPF_DMA_TDRI_XSIZE_REG        (ISP_GEPF_BASE + 0x0C)
#define GEPF_DMA_VERTICAL_FLIP_EN_REG  (ISP_GEPF_BASE + 0x10)
#define GEPF_DMA_DMA_SOFT_RESET_REG    (ISP_GEPF_BASE + 0x14)
#define GEPF_DMA_LAST_ULTRA_EN_REG     (ISP_GEPF_BASE + 0x18)
#define GEPF_DMA_SPECIAL_FUN_EN_REG    (ISP_GEPF_BASE + 0x1C)
#define GEPF_DMA_GEPFO_BASE_ADDR_REG   (ISP_GEPF_BASE + 0x30)
#define GEPF_DMA_GEPFO_OFST_ADDR_REG   (ISP_GEPF_BASE + 0x38)
#define GEPF_DMA_GEPFO_XSIZE_REG       (ISP_GEPF_BASE + 0x40)
#define GEPF_DMA_GEPFO_YSIZE_REG       (ISP_GEPF_BASE + 0x44)
#define GEPF_DMA_GEPFO_STRIDE_REG      (ISP_GEPF_BASE + 0x48)
#define GEPF_DMA_GEPFO_CON_REG         (ISP_GEPF_BASE + 0x4C)
#define GEPF_DMA_GEPFO_CON2_REG        (ISP_GEPF_BASE + 0x50)
#define GEPF_DMA_GEPFO_CON3_REG        (ISP_GEPF_BASE + 0x54)
#define GEPF_DMA_GEPFO_CROP_REG        (ISP_GEPF_BASE + 0x58)
#define GEPF_DMA_GEPF2O_BASE_ADDR_REG  (ISP_GEPF_BASE + 0x60)
#define GEPF_DMA_GEPF2O_OFST_ADDR_REG  (ISP_GEPF_BASE + 0x68)
#define GEPF_DMA_GEPF2O_XSIZE_REG      (ISP_GEPF_BASE + 0x70)
#define GEPF_DMA_GEPF2O_YSIZE_REG      (ISP_GEPF_BASE + 0x74)
#define GEPF_DMA_GEPF2O_STRIDE_REG     (ISP_GEPF_BASE + 0x78)
#define GEPF_DMA_GEPF2O_CON_REG        (ISP_GEPF_BASE + 0x7C)
#define GEPF_DMA_GEPF2O_CON2_REG       (ISP_GEPF_BASE + 0x80)
#define GEPF_DMA_GEPF2O_CON3_REG       (ISP_GEPF_BASE + 0x84)
#define GEPF_DMA_GEPF2O_CROP_REG       (ISP_GEPF_BASE + 0x88)
#define GEPF_DMA_GEPFI_BASE_ADDR_REG   (ISP_GEPF_BASE + 0x90)
#define GEPF_DMA_GEPFI_OFST_ADDR_REG   (ISP_GEPF_BASE + 0x98)
#define GEPF_DMA_GEPFI_XSIZE_REG       (ISP_GEPF_BASE + 0xA0)
#define GEPF_DMA_GEPFI_YSIZE_REG       (ISP_GEPF_BASE + 0xA4)
#define GEPF_DMA_GEPFI_STRIDE_REG      (ISP_GEPF_BASE + 0xA8)
#define GEPF_DMA_GEPFI_CON_REG         (ISP_GEPF_BASE + 0xAC)
#define GEPF_DMA_GEPFI_CON2_REG        (ISP_GEPF_BASE + 0xB0)
#define GEPF_DMA_GEPFI_CON3_REG        (ISP_GEPF_BASE + 0xB4)
#define GEPF_DMA_GEPF2I_BASE_ADDR_REG  (ISP_GEPF_BASE + 0xC0)
#define GEPF_DMA_GEPF2I_OFST_ADDR_REG  (ISP_GEPF_BASE + 0xC8)
#define GEPF_DMA_GEPF2I_XSIZE_REG      (ISP_GEPF_BASE + 0xD0)
#define GEPF_DMA_GEPF2I_YSIZE_REG      (ISP_GEPF_BASE + 0xD4)
#define GEPF_DMA_GEPF2I_STRIDE_REG     (ISP_GEPF_BASE + 0xD8)
#define GEPF_DMA_GEPF2I_CON_REG        (ISP_GEPF_BASE + 0xDC)
#define GEPF_DMA_GEPF2I_CON2_REG       (ISP_GEPF_BASE + 0xE0)
#define GEPF_DMA_GEPF2I_CON3_REG       (ISP_GEPF_BASE + 0xE4)
#define GEPF_DMA_GEPF3I_BASE_ADDR_REG  (ISP_GEPF_BASE + 0xF0)
#define GEPF_DMA_GEPF3I_OFST_ADDR_REG  (ISP_GEPF_BASE + 0xF8)
#define GEPF_DMA_GEPF3I_XSIZE_REG      (ISP_GEPF_BASE + 0x100)
#define GEPF_DMA_GEPF3I_YSIZE_REG      (ISP_GEPF_BASE + 0x104)
#define GEPF_DMA_GEPF3I_STRIDE_REG     (ISP_GEPF_BASE + 0x108)
#define GEPF_DMA_GEPF3I_CON_REG        (ISP_GEPF_BASE + 0x10C)
#define GEPF_DMA_GEPF3I_CON2_REG       (ISP_GEPF_BASE + 0x110)
#define GEPF_DMA_GEPF3I_CON3_REG       (ISP_GEPF_BASE + 0x114)
#define GEPF_DMA_GEPF4I_BASE_ADDR_REG  (ISP_GEPF_BASE + 0x120)
#define GEPF_DMA_GEPF4I_OFST_ADDR_REG  (ISP_GEPF_BASE + 0x128)
#define GEPF_DMA_GEPF4I_XSIZE_REG      (ISP_GEPF_BASE + 0x130)
#define GEPF_DMA_GEPF4I_YSIZE_REG      (ISP_GEPF_BASE + 0x134)
#define GEPF_DMA_GEPF4I_STRIDE_REG     (ISP_GEPF_BASE + 0x138)
#define GEPF_DMA_GEPF4I_CON_REG        (ISP_GEPF_BASE + 0x13C)
#define GEPF_DMA_GEPF4I_CON2_REG       (ISP_GEPF_BASE + 0x140)
#define GEPF_DMA_GEPF4I_CON3_REG       (ISP_GEPF_BASE + 0x144)
#define GEPF_DMA_DMA_ERR_CTRL_REG      (ISP_GEPF_BASE + 0x150)
#define GEPF_DMA_GEPFO_ERR_STAT_REG    (ISP_GEPF_BASE + 0x154)
#define GEPF_DMA_GEPF2O_ERR_STAT_REG   (ISP_GEPF_BASE + 0x158)
#define GEPF_DMA_GEPFI_ERR_STAT_REG    (ISP_GEPF_BASE + 0x15C)
#define GEPF_DMA_GEPF2I_ERR_STAT_REG   (ISP_GEPF_BASE + 0x160)
#define GEPF_DMA_GEPF3I_ERR_STAT_REG   (ISP_GEPF_BASE + 0x164)
#define GEPF_DMA_GEPF4I_ERR_STAT_REG   (ISP_GEPF_BASE + 0x168)
#define GEPF_DMA_DMA_DEBUG_ADDR_REG    (ISP_GEPF_BASE + 0x16C)
#define GEPF_DMA_DMA_RSV1_REG          (ISP_GEPF_BASE + 0x170)
#define GEPF_DMA_DMA_RSV2_REG          (ISP_GEPF_BASE + 0x174)
#define GEPF_DMA_DMA_RSV3_REG          (ISP_GEPF_BASE + 0x178)
#define GEPF_DMA_DMA_RSV4_REG          (ISP_GEPF_BASE + 0x17C)
#define GEPF_DMA_DMA_DEBUG_SEL_REG     (ISP_GEPF_BASE + 0x180)
#define GEPF_DMA_DMA_BW_SELF_TEST_REG  (ISP_GEPF_BASE + 0x184)

#define GEPF_IRQ_REG                   (ISP_GEPF_BASE + 0x400)
#define GEPF_CRT_0_REG                 (ISP_GEPF_BASE + 0x404)
#define GEPF_CRT_1_REG                 (ISP_GEPF_BASE + 0x408)
#define GEPF_CRT_2_REG                 (ISP_GEPF_BASE + 0x48C)
#define GEPF_FOCUS_BASE_ADDR_REG       (ISP_GEPF_BASE + 0x40C)
#define GEPF_FOCUS_OFFSET_REG          (ISP_GEPF_BASE + 0x410)
#define GEPF_Y_BASE_ADDR_REG           (ISP_GEPF_BASE + 0x414)
#define GEPF_YUV_IMG_SIZE_REG          (ISP_GEPF_BASE + 0x418)
#define GEPF_UV_BASE_ADDR_REG          (ISP_GEPF_BASE + 0x41C)
#define GEPF_DEPTH_BASE_ADDR_REG       (ISP_GEPF_BASE + 0x420)
#define GEPF_DEPTH_IMG_SIZE_REG        (ISP_GEPF_BASE + 0x428)
#define GEPF_BLUR_BASE_ADDR_REG        (ISP_GEPF_BASE + 0x42C)
#define GEPF_YUV_BASE_ADDR_REG         (ISP_GEPF_BASE + 0x430)
#define TEMP_CTR_0_REG                 (ISP_GEPF_BASE + 0x434)
#define TEMP_YUV_IMG_SIZE_REG          (ISP_GEPF_BASE + 0x438)
#define TEMP_DEPTH_IMG_SIZE_REG        (ISP_GEPF_BASE + 0x43C)
#define TEMP_PRE_Y_BASE_ADDR_REG       (ISP_GEPF_BASE + 0x440)
#define TEMP_Y_BASE_ADDR_REG           (ISP_GEPF_BASE + 0x444)
#define TEMP_DEPTH_BASE_ADDR_REG       (ISP_GEPF_BASE + 0x448)
#define TEMP_PRE_DEPTH_BASE_ADDR_REG   (ISP_GEPF_BASE + 0x44C)
#define BYPASS_DEPTH_BASE_ADDR_REG     (ISP_GEPF_BASE + 0x450)
#define BYPASS_DEPTH_WR_BASE_ADDR_REG  (ISP_GEPF_BASE + 0x454)
#define BYPASS_BLUR_BASE_ADDR_REG      (ISP_GEPF_BASE + 0x458)
#define BYPASS_BLUR_WR_BASE_ADDR_REG   (ISP_GEPF_BASE + 0x45C)
#define TEMP_BLUR_BASE_ADDR_REG        (ISP_GEPF_BASE + 0x460)
#define TEMP_PRE_BLUR_BASE_ADDR_REG    (ISP_GEPF_BASE + 0x464)
#define TEMP_BLUR_WR_BASE_ADDR_REG     (ISP_GEPF_BASE + 0x468)
#define TEMP_DEPTH_WR_BASE_ADDR_REG    (ISP_GEPF_BASE + 0x46C)
#define GEPF_DEPTH_WR_BASE_ADDR_REG    (ISP_GEPF_BASE + 0x470)
#define GEPF_BLUR_WR_BASE_ADDR_REG     (ISP_GEPF_BASE + 0x474)
#define GEPF_DMA_CTL_REG               (ISP_GEPF_BASE + 0x478)
#define GEPF_DMA_RST_REG               (ISP_GEPF_BASE + 0x47C)
#define GEPF_DMA_DCM_REG               (ISP_GEPF_BASE + 0x480)
#define GEPF_MODULE_DCM_REG            (ISP_GEPF_BASE + 0x484)
#define TEMP_CTR_1_REG                 (ISP_GEPF_BASE + 0x488)
#define GEPF_480_Y_ADDR_REG            (ISP_GEPF_BASE + 0x490)
#define GEPF_480_UV_ADDR_REG           (ISP_GEPF_BASE + 0x494)
#define GEPF_STRIDE_Y_UV_REG           (ISP_GEPF_BASE + 0x498)
#define GEPF_STRIDE_DEPTH_REG          (ISP_GEPF_BASE + 0x49C)
#define TEMP_STRIDE_DEPTH_REG          (ISP_GEPF_BASE + 0x4A0)
#define GEPF_STRIDE_Y_UV_480_REG       (ISP_GEPF_BASE + 0x4A4)
#define DMA_DEBUG_DATA_REG             (ISP_GEPF_BASE + 0x4A8)
#define DMA_DCM_ST_REG                 (ISP_GEPF_BASE + 0x4AC)
#define DMA_RDY_ST_REG                 (ISP_GEPF_BASE + 0x4B0)
#define DMA_REQ_ST_REG                 (ISP_GEPF_BASE + 0x4B4)
#define GEPF_DEBUG_SEL_REG             (ISP_GEPF_BASE + 0x4C0)
#define GEPF_DEBUG_0_REG               (ISP_GEPF_BASE + 0x4C4)
#define GEPF_DEBUG_1_REG               (ISP_GEPF_BASE + 0x4C8)
#define GEPF_MEM_STALL_CNT_REG         (ISP_GEPF_BASE + 0x4CC)

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 GEPF_MsToJiffies(MUINT32 Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 GEPF_UsToJiffies(MUINT32 Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 GEPF_GetIRQState(MUINT32 type, MUINT32 userNumber, MUINT32 stus,
				      GEPF_PROCESS_ID_ENUM whichReq, int ProcessID)
{
	MUINT32 ret = 0;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */

	/*  */
	spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[type]), flags);
#ifdef GEPF_USE_GCE

#ifdef GEPF_MULTIPROCESS_TIMEING_ISSUE
	if (stus & GEPF_INT_ST) {
		ret = ((GEPFInfo.IrqInfo.GepfIrqCnt > 0)
		       && (GEPFInfo.ProcessID[GEPFInfo.ReadReqIdx] == ProcessID));
	} else {
		LOG_ERR
		    (" WaitIRQ StatusErr, type:%d, userNum:%d, status:%d, whichReq:%d,ProcessID:0x%x, ReadReqIdx:%d\n",
		     type, userNumber, stus, whichReq, ProcessID, GEPFInfo.ReadReqIdx);
	}

#else
	if (stus & GEPF_INT_ST) {
		ret = ((GEPFInfo.IrqInfo.GepfIrqCnt > 0)
		       && (GEPFInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
	} else {
		LOG_ERR
		    ("WaitIRQ Status Error, type:%d, userNumber:%d, status:%d, whichReq:%d, ProcessID:0x%x\n",
		     type, userNumber, stus, whichReq, ProcessID);
	}
#endif
#else
	ret = ((GEPFInfo.IrqInfo.Status[type] & stus)
	       && (GEPFInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
#endif
	spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[type]), flags);
	/*  */
	return ret;
}


/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 GEPF_JiffiesToMs(MUINT32 Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}


#define RegDump(start, end) {\
	MUINT32 i;\
	for (i = start; i <= end; i += 0x10) {\
		LOG_DBG("[0x%08X %08X],[0x%08X %08X],[0x%08X %08X],[0x%08X %08X]",\
	    (unsigned int)(ISP_GEPF_BASE + i), (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i),\
	    (unsigned int)(ISP_GEPF_BASE + i+0x4), (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i+0x4),\
	    (unsigned int)(ISP_GEPF_BASE + i+0x8), (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i+0x8),\
	    (unsigned int)(ISP_GEPF_BASE + i+0xc), (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i+0xc));\
	} \
}


static MBOOL ConfigGEPFRequest(MINT32 ReqIdx)
{
#ifdef GEPF_USE_GCE
	MUINT32 j;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */


	spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
	if (g_GEPF_ReqRing.GEPFReq_Struct[ReqIdx].State == GEPF_REQUEST_STATE_PENDING) {
		g_GEPF_ReqRing.GEPFReq_Struct[ReqIdx].State = GEPF_REQUEST_STATE_RUNNING;
		for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; j++) {
			if (GEPF_FRAME_STATUS_ENQUE ==
			    g_GEPF_ReqRing.GEPFReq_Struct[ReqIdx].GepfFrameStatus[j]) {
				g_GEPF_ReqRing.GEPFReq_Struct[ReqIdx].GepfFrameStatus[j] =
				    GEPF_FRAME_STATUS_RUNNING;
				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);
				ConfigGEPFHW(&g_GEPF_ReqRing.GEPFReq_Struct[ReqIdx].
					     GepfFrameConfig[j]);
				spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						  flags);
			}
		}
	} else {
		LOG_ERR("ConfigGEPFRequest state machine error!!, ReqIdx:%d, State:%d\n",
			ReqIdx, g_GEPF_ReqRing.GEPFReq_Struct[ReqIdx].State);
	}
	spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);


	return MTRUE;
#else
	LOG_ERR("ConfigGEPFRequest don't support this mode.!!\n");
	return MFALSE;
#endif
}


static MBOOL ConfigGEPF(void)
{
#ifdef GEPF_USE_GCE

	MUINT32 i, j, k;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */


	spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
	for (k = 0; k < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; k++) {
		i = (g_GEPF_ReqRing.HWProcessIdx + k) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
		if (g_GEPF_ReqRing.GEPFReq_Struct[i].State == GEPF_REQUEST_STATE_PENDING) {
			g_GEPF_ReqRing.GEPFReq_Struct[i].State =
			    GEPF_REQUEST_STATE_RUNNING;
			for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; j++) {
				if (GEPF_FRAME_STATUS_ENQUE ==
				    g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]) {
					/* break; */
					g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j] =
					    GEPF_FRAME_STATUS_RUNNING;
					spin_unlock_irqrestore(&
							       (GEPFInfo.
								SpinLockIrq
								[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
					ConfigGEPFHW(&g_GEPF_ReqRing.GEPFReq_Struct[i].
						     GepfFrameConfig[j]);
					spin_lock_irqsave(&
							  (GEPFInfo.
							   SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
							  flags);
				}
			}
			/* LOG_DBG("ConfigGEPF idx j:%d\n",j); */
			if (j != _SUPPORT_MAX_GEPF_FRAME_REQUEST_) {
				LOG_ERR
				    ("GEPF Config State is wrong!  idx j(%d), HWProcessIdx(%d), State(%d)\n",
				     j, g_GEPF_ReqRing.HWProcessIdx,
				     g_GEPF_ReqRing.GEPFReq_Struct[i].State);
				/* g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]
				* = GEPF_FRAME_STATUS_RUNNING;
				*/
				/* spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags); */
				/* ConfigGEPFHW(&g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameConfig[j]); */
				/* return MTRUE; */
				return MFALSE;
			}
			/*else {
			*	g_GEPF_ReqRing.GEPFReq_Struct[i].State = GEPF_REQUEST_STATE_RUNNING;
			*	LOG_ERR("GEPF Config State is wrong! HWProcessIdx(%d), State(%d)\n",
			*	g_GEPF_ReqRing.HWProcessIdx, g_GEPF_ReqRing.GEPFReq_Struct[i].State);
			*	g_GEPF_ReqRing.HWProcessIdx = (g_GEPF_ReqRing.HWProcessIdx+1)
			*	%_SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
			*}
			*/
		}
	}
	spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
	if (k == _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_)
		LOG_DBG("No any GEPF Request in Ring!!\n");

	return MTRUE;


#else				/* #ifdef GEPF_USE_GCE */

	MUINT32 i, j, k;
	MUINT32 flags;

	for (k = 0; k < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; k++) {
		i = (g_GEPF_ReqRing.HWProcessIdx + k) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
		if (g_GEPF_ReqRing.GEPFReq_Struct[i].State == GEPF_REQUEST_STATE_PENDING) {
			for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; j++) {
				if (GEPF_FRAME_STATUS_ENQUE ==
				    g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]) {
					break;
				}
			}
			LOG_DBG("ConfigGEPF idx j:%d\n", j);
			if (j != _SUPPORT_MAX_GEPF_FRAME_REQUEST_) {
				g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j] =
				    GEPF_FRAME_STATUS_RUNNING;
				ConfigGEPFHW(&g_GEPF_ReqRing.GEPFReq_Struct[i].
					     GepfFrameConfig[j]);
				return MTRUE;
			}
			/*else {*/
			LOG_ERR
			    ("GEPF Config State is wrong! HWProcessIdx(%d), State(%d)\n",
			     g_GEPF_ReqRing.HWProcessIdx,
			     g_GEPF_ReqRing.GEPFReq_Struct[i].State);
			g_GEPF_ReqRing.HWProcessIdx =
			    (g_GEPF_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
			/*}*/
		}
	}
	if (k == _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_)
		LOG_DBG("No any GEPF Request in Ring!!\n");

	return MFALSE;

#endif				/* #ifdef GEPF_USE_GCE */



}


static MBOOL UpdateGEPF(pid_t *ProcessID)
{
#ifdef GEPF_USE_GCE
	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_GEPF_ReqRing.HWProcessIdx; i < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; i++) {
		if (g_GEPF_ReqRing.GEPFReq_Struct[i].State == GEPF_REQUEST_STATE_RUNNING) {
			for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; j++) {
				if (GEPF_FRAME_STATUS_RUNNING ==
				    g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateGEPF idx j:%d\n", j);
			if (j != _SUPPORT_MAX_GEPF_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j] =
				    GEPF_FRAME_STATUS_FINISHED;
				if ((_SUPPORT_MAX_GEPF_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_GEPF_FRAME_REQUEST_ > (next_idx))
					&& (GEPF_FRAME_STATUS_EMPTY ==
					    g_GEPF_ReqRing.GEPFReq_Struct[i].
					    GepfFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) =
					    g_GEPF_ReqRing.GEPFReq_Struct[i].processID;
					g_GEPF_ReqRing.GEPFReq_Struct[i].State =
					    GEPF_REQUEST_STATE_FINISHED;
					g_GEPF_ReqRing.HWProcessIdx =
					    (g_GEPF_ReqRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish GEPF Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_GEPF_ReqRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish GEPF Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_GEPF_ReqRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_ERR,
				       "GEPF State Machine is wrong! HWProcessIdx(%d), State(%d)\n",
				       g_GEPF_ReqRing.HWProcessIdx,
				       g_GEPF_ReqRing.GEPFReq_Struct[i].State);
			g_GEPF_ReqRing.GEPFReq_Struct[i].State =
			    GEPF_REQUEST_STATE_FINISHED;
			g_GEPF_ReqRing.HWProcessIdx =
			    (g_GEPF_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}

	return bFinishRequest;


#else				/* #ifdef GEPF_USE_GCE */
	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_GEPF_ReqRing.HWProcessIdx; i < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; i++) {
		if (g_GEPF_ReqRing.GEPFReq_Struct[i].State == GEPF_REQUEST_STATE_PENDING) {
			for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; j++) {
				if (GEPF_FRAME_STATUS_RUNNING ==
				    g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateGEPF idx j:%d\n", j);
			if (j != _SUPPORT_MAX_GEPF_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j] =
				    GEPF_FRAME_STATUS_FINISHED;
				if ((_SUPPORT_MAX_GEPF_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_GEPF_FRAME_REQUEST_ > (next_idx))
					&& (GEPF_FRAME_STATUS_EMPTY ==
					    g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) =
					    g_GEPF_ReqRing.GEPFReq_Struct[i].processID;
					g_GEPF_ReqRing.GEPFReq_Struct[i].State =
					    GEPF_REQUEST_STATE_FINISHED;
					g_GEPF_ReqRing.HWProcessIdx =
					    (g_GEPF_ReqRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish GEPF Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_GEPF_ReqRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish GEPF Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_GEPF_ReqRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_ERR,
				       "GEPF State Machine is wrong! HWProcessIdx(%d), State(%d)\n",
				       g_GEPF_ReqRing.HWProcessIdx,
				       g_GEPF_ReqRing.GEPFReq_Struct[i].State);
			g_GEPF_ReqRing.GEPFReq_Struct[i].State =
			    GEPF_REQUEST_STATE_FINISHED;
			g_GEPF_ReqRing.HWProcessIdx =
			    (g_GEPF_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}

	return bFinishRequest;

#endif				/* #ifdef GEPF_USE_GCE */


}

static MINT32 ConfigGEPFHW(GEPF_Config *pGepfConfig)
#if !BYPASS_REG
{
#ifdef GEPF_USE_GCE
		struct cmdqRecStruct *handle;	/* kernel-4.4 usage */
		/* cmdqRecHandle handle; */ /* kernel-3.18 usage */
		uint64_t engineFlag = (1L << CMDQ_ENG_GEPF);
#endif

	if (GEPF_DBG_DBGLOG == (GEPF_DBG_DBGLOG & GEPFInfo.DebugMask)) {

		LOG_ERR("ConfigGEPFHW Start!\n");
		LOG_ERR("GEPF_CRT_0:0x%x!\n", pGepfConfig->GEPF_CRT_0);
		LOG_ERR("GEPF_CRT_1:0x%x!\n", pGepfConfig->GEPF_CRT_1);
		LOG_ERR("GEPF_CRT_2:0x%x!\n", pGepfConfig->GEPF_CRT_2);
		LOG_ERR("GEPF_FOCUS_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_FOCUS_BASE_ADDR);
		LOG_ERR("GEPF_FOCUS_OFFSET:0x%x!\n", pGepfConfig->GEPF_FOCUS_OFFSET);
		LOG_ERR("GEPF_Y_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_Y_BASE_ADDR);
		LOG_ERR("GEPF_YUV_IMG_SIZE:0x%x!\n", pGepfConfig->GEPF_YUV_IMG_SIZE);
		LOG_ERR("GEPF_UV_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_UV_BASE_ADDR);
		LOG_ERR("GEPF_DEPTH_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_DEPTH_BASE_ADDR);
		LOG_ERR("GEPF_DEPTH_IMG_SIZE:0x%x!\n", pGepfConfig->GEPF_DEPTH_IMG_SIZE);
		LOG_ERR("GEPF_BLUR_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_BLUR_BASE_ADDR);
		LOG_ERR("GEPF_YUV_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_YUV_BASE_ADDR);
		LOG_ERR("TEMP_PRE_Y_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_PRE_Y_BASE_ADDR);
		LOG_ERR("TEMP_Y_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_Y_BASE_ADDR);
		LOG_ERR("TEMP_DEPTH_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_DEPTH_BASE_ADDR);
		LOG_ERR("TEMP_PRE_DEPTH_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_PRE_DEPTH_BASE_ADDR);
		LOG_ERR("BYPASS_DEPTH_BASE_ADDR:0x%x!\n", pGepfConfig->BYPASS_DEPTH_BASE_ADDR);
		LOG_ERR("BYPASS_DEPTH_WR_BASE_ADDR:0x%x!\n", pGepfConfig->BYPASS_DEPTH_WR_BASE_ADDR);
		LOG_ERR("BYPASS_BLUR_BASE_ADDR:0x%x!\n", pGepfConfig->BYPASS_BLUR_BASE_ADDR);
		LOG_ERR("BYPASS_BLUR_WR_BASE_ADDR:0x%x!\n", pGepfConfig->BYPASS_BLUR_WR_BASE_ADDR);
		LOG_ERR("TEMP_BLUR_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_BLUR_BASE_ADDR);
		LOG_ERR("TEMP_PRE_BLUR_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_PRE_BLUR_BASE_ADDR);
		LOG_ERR("TEMP_BLUR_WR_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_BLUR_WR_BASE_ADDR);
		LOG_ERR("TEMP_DEPTH_WR_BASE_ADDR:0x%x!\n", pGepfConfig->TEMP_DEPTH_WR_BASE_ADDR);
		LOG_ERR("GEPF_DEPTH_WR_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_DEPTH_WR_BASE_ADDR);
		LOG_ERR("GEPF_BLUR_WR_BASE_ADDR:0x%x!\n", pGepfConfig->GEPF_BLUR_WR_BASE_ADDR);
		LOG_ERR("TEMP_CTR_1:0x%x!\n", pGepfConfig->TEMP_CTR_1);
		LOG_ERR("GEPF_480_Y_ADDR:0x%x!\n", pGepfConfig->GEPF_480_Y_ADDR);
		LOG_ERR("GEPF_480_UV_ADDR:0x%x!\n", pGepfConfig->GEPF_480_UV_ADDR);
		LOG_ERR("GEPF_STRIDE_Y_UV:0x%x!\n", pGepfConfig->GEPF_STRIDE_Y_UV);
		LOG_ERR("GEPF_STRIDE_DEPTH:0x%x!\n", pGepfConfig->GEPF_STRIDE_DEPTH);
		LOG_ERR("TEMP_STRIDE_DEPTH:0x%x!\n", pGepfConfig->TEMP_STRIDE_DEPTH);
		LOG_ERR("GEPF_STRIDE_Y_UV_480:0x%x!\n", pGepfConfig->GEPF_STRIDE_Y_UV_480);
	}
#ifdef GEPF_USE_GCE

#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigGEPFHW");
#endif

	cmdqRecCreate(CMDQ_SCENARIO_KERNEL_CONFIG_GENERAL, &handle);
	/* CMDQ driver dispatches CMDQ HW thread and HW thread's priority according to scenario */

	cmdqRecSetEngine(handle, engineFlag);

	cmdqRecReset(handle);

	/* Use command queue to write register */
	cmdqRecWrite(handle, GEPF_CRT_0_HW, pGepfConfig->GEPF_CRT_0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_CRT_1_HW, pGepfConfig->GEPF_CRT_1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_CRT_2_HW, (pGepfConfig->GEPF_CRT_2 | 0x20000), CMDQ_REG_MASK); /* GEPF Irq Enable */
	cmdqRecWrite(handle, GEPF_FOCUS_BASE_ADDR_HW, pGepfConfig->GEPF_FOCUS_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_FOCUS_OFFSET_HW, pGepfConfig->GEPF_FOCUS_OFFSET, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_Y_BASE_ADDR_HW, pGepfConfig->GEPF_Y_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_YUV_IMG_SIZE_HW, pGepfConfig->GEPF_YUV_IMG_SIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_UV_BASE_ADDR_HW, pGepfConfig->GEPF_UV_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_DEPTH_BASE_ADDR_HW, pGepfConfig->GEPF_DEPTH_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_DEPTH_IMG_SIZE_HW, pGepfConfig->GEPF_DEPTH_IMG_SIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_BLUR_BASE_ADDR_HW, pGepfConfig->GEPF_BLUR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_YUV_BASE_ADDR_HW, pGepfConfig->GEPF_YUV_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_PRE_Y_BASE_ADDR_HW, pGepfConfig->TEMP_PRE_Y_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_Y_BASE_ADDR_HW, pGepfConfig->TEMP_Y_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_DEPTH_BASE_ADDR_HW, pGepfConfig->TEMP_DEPTH_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_PRE_DEPTH_BASE_ADDR_HW, pGepfConfig->TEMP_PRE_DEPTH_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, BYPASS_DEPTH_BASE_ADDR_HW, pGepfConfig->BYPASS_DEPTH_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, BYPASS_DEPTH_WR_BASE_ADDR_HW, pGepfConfig->BYPASS_DEPTH_WR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, BYPASS_BLUR_BASE_ADDR_HW, pGepfConfig->BYPASS_BLUR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, BYPASS_BLUR_WR_BASE_ADDR_HW, pGepfConfig->BYPASS_BLUR_WR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_BLUR_BASE_ADDR_HW, pGepfConfig->TEMP_BLUR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_PRE_BLUR_BASE_ADDR_HW, pGepfConfig->TEMP_PRE_BLUR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_BLUR_WR_BASE_ADDR_HW, pGepfConfig->TEMP_BLUR_WR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_DEPTH_WR_BASE_ADDR_HW, pGepfConfig->TEMP_DEPTH_WR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_DEPTH_WR_BASE_ADDR_HW, pGepfConfig->GEPF_DEPTH_WR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_BLUR_WR_BASE_ADDR_HW, pGepfConfig->GEPF_BLUR_WR_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_CTR_1_HW, pGepfConfig->TEMP_CTR_1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_480_Y_ADDR_HW, pGepfConfig->GEPF_480_Y_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_480_UV_ADDR_HW, pGepfConfig->GEPF_480_UV_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_STRIDE_Y_UV_HW, pGepfConfig->GEPF_STRIDE_Y_UV, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_STRIDE_DEPTH_HW, pGepfConfig->GEPF_STRIDE_DEPTH, CMDQ_REG_MASK);
	cmdqRecWrite(handle, TEMP_STRIDE_DEPTH_HW, pGepfConfig->TEMP_STRIDE_DEPTH, CMDQ_REG_MASK);
	cmdqRecWrite(handle, GEPF_STRIDE_Y_UV_480_HW, pGepfConfig->GEPF_STRIDE_Y_UV_480, CMDQ_REG_MASK);

	cmdqRecWrite(handle, GEPF_DMA_CTL_HW, 0x3F, CMDQ_REG_MASK);                               /* GEPF DMA Enable */
	cmdqRecWrite(handle, GEPF_MODULE_DCM_HW, 0x10, CMDQ_REG_MASK);	                            /* GEPF DCM Start */
	cmdqRecWrite(handle, GEPF_CRT_0_HW, (pGepfConfig->GEPF_CRT_0 | 0x1), CMDQ_REG_MASK);        /* GEPF enable */

	cmdqRecWait(handle, CMDQ_EVENT_GEPF_EOF);
	cmdqRecWrite(handle, GEPF_CRT_0_HW, (pGepfConfig->GEPF_CRT_0 & 0xFFFFFFFE), CMDQ_REG_MASK); /* GEPF disable */
	cmdqRecWrite(handle, GEPF_MODULE_DCM_HW, 0x0, CMDQ_REG_MASK);	                   /* GEPF DCM Start Disable */

	/* non-blocking API, Please  use cmdqRecFlushAsync() */
	cmdqRecFlushAsync(handle);
	cmdqRecReset(handle);	/* if you want to re-use the handle, please reset the handle */
	cmdqRecDestroy(handle);	/* recycle the memory */

#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#else

#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigGEPFHW");
#endif

	GEPF_WR32(GEPF_CRT_0_REG, pGepfConfig->GEPF_CRT_0);
	GEPF_WR32(GEPF_CRT_1_REG, pGepfConfig->GEPF_CRT_1);
	GEPF_WR32(GEPF_CRT_2_REG, (pGepfConfig->GEPF_CRT_2 | 0x20000)); /* GEPF Irq Enable */
	GEPF_WR32(GEPF_FOCUS_BASE_ADDR_REG, pGepfConfig->GEPF_FOCUS_BASE_ADDR);
	GEPF_WR32(GEPF_FOCUS_OFFSET_REG, pGepfConfig->GEPF_FOCUS_OFFSET);
	GEPF_WR32(GEPF_Y_BASE_ADDR_REG, pGepfConfig->GEPF_Y_BASE_ADDR);
	GEPF_WR32(GEPF_YUV_IMG_SIZE_REG, pGepfConfig->GEPF_YUV_IMG_SIZE);
	GEPF_WR32(GEPF_UV_BASE_ADDR_REG, pGepfConfig->GEPF_UV_BASE_ADDR);
	GEPF_WR32(GEPF_DEPTH_BASE_ADDR_REG, pGepfConfig->GEPF_DEPTH_BASE_ADDR);
	GEPF_WR32(GEPF_DEPTH_IMG_SIZE_REG, pGepfConfig->GEPF_DEPTH_IMG_SIZE);
	GEPF_WR32(GEPF_BLUR_BASE_ADDR_REG, pGepfConfig->GEPF_BLUR_BASE_ADDR);
	GEPF_WR32(GEPF_YUV_BASE_ADDR_REG, pGepfConfig->GEPF_YUV_BASE_ADDR);
	GEPF_WR32(TEMP_PRE_Y_BASE_ADDR_REG, pGepfConfig->TEMP_PRE_Y_BASE_ADDR);
	GEPF_WR32(TEMP_Y_BASE_ADDR_REG, pGepfConfig->TEMP_Y_BASE_ADDR);
	GEPF_WR32(TEMP_DEPTH_BASE_ADDR_REG, pGepfConfig->TEMP_DEPTH_BASE_ADDR);
	GEPF_WR32(TEMP_PRE_DEPTH_BASE_ADDR_REG, pGepfConfig->TEMP_PRE_DEPTH_BASE_ADDR);
	GEPF_WR32(BYPASS_DEPTH_BASE_ADDR_REG, pGepfConfig->BYPASS_DEPTH_BASE_ADDR);
	GEPF_WR32(BYPASS_DEPTH_WR_BASE_ADDR_REG, pGepfConfig->BYPASS_DEPTH_WR_BASE_ADDR);
	GEPF_WR32(BYPASS_BLUR_BASE_ADDR_REG, pGepfConfig->BYPASS_BLUR_BASE_ADDR);
	GEPF_WR32(BYPASS_BLUR_WR_BASE_ADDR_REG, pGepfConfig->BYPASS_BLUR_WR_BASE_ADDR);
	GEPF_WR32(TEMP_BLUR_BASE_ADDR_REG, pGepfConfig->TEMP_BLUR_BASE_ADDR);
	GEPF_WR32(TEMP_PRE_BLUR_BASE_ADDR_REG, pGepfConfig->TEMP_PRE_BLUR_BASE_ADDR);
	GEPF_WR32(TEMP_BLUR_WR_BASE_ADDR_REG, pGepfConfig->TEMP_BLUR_WR_BASE_ADDR);
	GEPF_WR32(TEMP_DEPTH_WR_BASE_ADDR_REG, pGepfConfig->TEMP_DEPTH_WR_BASE_ADDR);
	GEPF_WR32(GEPF_DEPTH_WR_BASE_ADDR_REG, pGepfConfig->GEPF_DEPTH_WR_BASE_ADDR);
	GEPF_WR32(GEPF_BLUR_WR_BASE_ADDR_REG, pGepfConfig->GEPF_BLUR_WR_BASE_ADDR);
	GEPF_WR32(TEMP_CTR_1_REG, pGepfConfig->TEMP_CTR_1);
	GEPF_WR32(GEPF_480_Y_ADDR_REG, pGepfConfig->GEPF_480_Y_ADDR);
	GEPF_WR32(GEPF_480_UV_ADDR_REG, pGepfConfig->GEPF_480_UV_ADDR);
	GEPF_WR32(GEPF_STRIDE_Y_UV_REG, pGepfConfig->GEPF_STRIDE_Y_UV);
	GEPF_WR32(GEPF_STRIDE_DEPTH_REG, pGepfConfig->GEPF_STRIDE_DEPTH);
	GEPF_WR32(TEMP_STRIDE_DEPTH_REG, pGepfConfig->TEMP_STRIDE_DEPTH);
	GEPF_WR32(GEPF_STRIDE_Y_UV_480_REG, pGepfConfig->GEPF_STRIDE_Y_UV_480);

	GEPF_WR32(GEPF_DMA_CTL_REG, 0x3F);     /* GEPF DMA Enable */
	GEPF_WR32(GEPF_MODULE_DCM_REG, 0x10);  /* GEPF DCM Start */
	GEPF_WR32(GEPF_CRT_0_REG, (pGepfConfig->GEPF_CRT_0 | 0x1));  /* GEPF enable */

#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
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

#define GEPF_IS_BUSY    0x2

#ifndef GEPF_USE_GCE

static MBOOL Check_GEPF_Is_Busy(void)
#if !BYPASS_REG
{
	MUINT32 Ctrl_Fsm;
	MUINT32 Gepf_Start;

	/* Ctrl_Fsm = GEPF_RD32(RSC_DBG_INFO_00_REG);	// Daniel add need check with SY */
	Gepf_Start = GEPF_RD32(GEPF_IRQ_REG);
	if ((GEPF_IS_BUSY == (Ctrl_Fsm & GEPF_IS_BUSY)) || (GEPF_START == (Gepf_Start & GEPF_START)))
		return MTRUE;

	return MFALSE;
}
#else
{
	return MFALSE;
}
#endif
#endif


/*
 *
 */
static MINT32 GEPF_DumpReg(void)
#if BYPASS_REG
{
	return 0;
}
#else
{
	MINT32 Ret = 0;
	MUINT32 i, j;
	/*  */
	LOG_INF("- E.");
	/*  */
	LOG_INF("GEPF Config Info\n");
	/* GEPF Config0 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_IRQ_HW),
		(unsigned int)GEPF_RD32(GEPF_IRQ_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_CRT_0_HW),
		(unsigned int)GEPF_RD32(GEPF_CRT_0_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_CRT_1_HW),
		(unsigned int)GEPF_RD32(GEPF_CRT_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_CRT_2_HW),
		(unsigned int)GEPF_RD32(GEPF_CRT_2_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_FOCUS_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_FOCUS_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_FOCUS_OFFSET_HW),
		(unsigned int)GEPF_RD32(GEPF_FOCUS_OFFSET_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_Y_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_Y_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_YUV_IMG_SIZE_HW),
		(unsigned int)GEPF_RD32(GEPF_YUV_IMG_SIZE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_UV_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_UV_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_DEPTH_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_DEPTH_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_DEPTH_IMG_SIZE_HW),
		(unsigned int)GEPF_RD32(GEPF_DEPTH_IMG_SIZE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_BLUR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_BLUR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_YUV_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_YUV_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_PRE_Y_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_PRE_Y_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_Y_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_Y_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_DEPTH_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_DEPTH_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_PRE_DEPTH_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_PRE_DEPTH_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(BYPASS_DEPTH_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(BYPASS_DEPTH_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(BYPASS_DEPTH_WR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(BYPASS_DEPTH_WR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(BYPASS_BLUR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(BYPASS_BLUR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(BYPASS_BLUR_WR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(BYPASS_BLUR_WR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_BLUR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_BLUR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_PRE_BLUR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_PRE_BLUR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_BLUR_WR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_BLUR_WR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_DEPTH_WR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(TEMP_DEPTH_WR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_DEPTH_WR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_DEPTH_WR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_BLUR_WR_BASE_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_BLUR_WR_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_DMA_CTL_HW),
		(unsigned int)GEPF_RD32(GEPF_DMA_CTL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_DMA_CTL_HW),
		(unsigned int)GEPF_RD32(GEPF_DMA_CTL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_DMA_DCM_HW),
		(unsigned int)GEPF_RD32(GEPF_DMA_DCM_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_MODULE_DCM_HW),
		(unsigned int)GEPF_RD32(GEPF_MODULE_DCM_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_CTR_1_HW),
		(unsigned int)GEPF_RD32(TEMP_CTR_1_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_480_Y_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_480_Y_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_480_UV_ADDR_HW),
		(unsigned int)GEPF_RD32(GEPF_480_UV_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_STRIDE_Y_UV_HW),
		(unsigned int)GEPF_RD32(GEPF_STRIDE_Y_UV_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_STRIDE_DEPTH_HW),
		(unsigned int)GEPF_RD32(GEPF_STRIDE_DEPTH_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(TEMP_STRIDE_DEPTH_HW),
		(unsigned int)GEPF_RD32(TEMP_STRIDE_DEPTH_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(GEPF_STRIDE_Y_UV_480_HW),
		(unsigned int)GEPF_RD32(GEPF_STRIDE_Y_UV_480_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DMA_DEBUG_DATA_HW),
		(unsigned int)GEPF_RD32(DMA_DEBUG_DATA_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DMA_DCM_ST_HW),
		(unsigned int)GEPF_RD32(DMA_DCM_ST_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DMA_RDY_ST_HW),
		(unsigned int)GEPF_RD32(DMA_RDY_ST_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(DMA_REQ_ST_HW),
		(unsigned int)GEPF_RD32(DMA_REQ_ST_REG));

	LOG_INF("GEPF:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n", g_GEPF_ReqRing.HWProcessIdx,
		g_GEPF_ReqRing.WriteIdx, g_GEPF_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; i++) {
		LOG_INF
			("GEPF Req:State:%d, procID:0x%08X, callerID:0x%08X, enqReqNum:%d, FraWRIdx:%d, FraRDIdx:%d\n",
		     g_GEPF_ReqRing.GEPFReq_Struct[i].State, g_GEPF_ReqRing.GEPFReq_Struct[i].processID,
		     g_GEPF_ReqRing.GEPFReq_Struct[i].callerID, g_GEPF_ReqRing.GEPFReq_Struct[i].enqueReqNum,
		     g_GEPF_ReqRing.GEPFReq_Struct[i].FrameWRIdx, g_GEPF_ReqRing.GEPFReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_;) {
			LOG_INF
			    ("GEPF:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
			     j, g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]
			     , j + 1, g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j + 1],
			     j + 2, g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j + 2]
			     , j + 3, g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j + 3]);
			j = j + 4;
		}

	}

	LOG_INF("- X.");
	/*  */
	return Ret;
}
#endif
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
static inline void GEPF_Prepare_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> GEPF clk */
	ret = clk_prepare(gepf_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot prepare CG_SCP_SYS_MM0 clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_2X clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_GALS_M0_2X clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_GALS_M1_2X clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_UPSZ0 clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_UPSZ1 clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_FIFO0 clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_FIFO1 clock\n");

	ret = clk_prepare(gepf_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_LARB5 clock\n");

	ret = clk_prepare(gepf_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot prepare CG_SCP_SYS_ISP clock\n");

	ret = clk_prepare(gepf_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot prepare CG_IMGSYS_LARB clock\n");

	ret = clk_prepare(gepf_clk.CG_IMGSYS_GEPF);
	if (ret)
		LOG_ERR("cannot prepare CG_IMGSYS_GEPF clock\n");

}

static inline void GEPF_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> GEPF  clk */
	ret = clk_enable(gepf_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot enable CG_SCP_SYS_MM0 clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_2X clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_GALS_M0_2X clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_GALS_M1_2X clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_UPSZ0 clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_UPSZ1 clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_FIFO0 clock\n");

	ret = clk_enable(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_FIFO1 clock\n");

	ret = clk_enable(gepf_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot enable CG_MM_LARB5 clock\n");


	ret = clk_enable(gepf_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot enable CG_SCP_SYS_ISP clock\n");

	ret = clk_enable(gepf_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot enable CG_IMGSYS_LARB clock\n");

	ret = clk_enable(gepf_clk.CG_IMGSYS_GEPF);
	if (ret)
		LOG_ERR("cannot enable CG_IMGSYS_GEPF clock\n");

}
#define SMI_CLK
static inline void GEPF_Prepare_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> GEPF clk */
#ifndef SMI_CLK
	ret = clk_prepare_enable(gepf_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_SCP_SYS_MM0 clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_2X clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_GALS_M0_2X clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_GALS_M1_2X clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_UPSZ0 clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_UPSZ1 clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_FIFO0 clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_FIFO1 clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_LARB5 clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_SCP_SYS_ISP clock\n");

	ret = clk_prepare_enable(gepf_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_LARB clock\n");
#else
	smi_bus_enable(SMI_LARB_IMGSYS1, "camera_gepf");
#endif
	ret = clk_prepare_enable(gepf_clk.CG_IMGSYS_GEPF);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_GEPF clock\n");

}

static inline void GEPF_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: GEPF clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0 */
	clk_unprepare(gepf_clk.CG_IMGSYS_GEPF);
	clk_unprepare(gepf_clk.CG_IMGSYS_LARB);
	clk_unprepare(gepf_clk.CG_SCP_SYS_ISP);
	clk_unprepare(gepf_clk.CG_MM_LARB5);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON_2X);
	clk_unprepare(gepf_clk.CG_MM_SMI_COMMON);
	clk_unprepare(gepf_clk.CG_SCP_SYS_MM0);
}

static inline void GEPF_Disable_ccf_clock(void)
{
	/* must keep this clk close order: GEPF clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0 */
	clk_disable(gepf_clk.CG_IMGSYS_GEPF);
	clk_disable(gepf_clk.CG_IMGSYS_LARB);
	clk_disable(gepf_clk.CG_SCP_SYS_ISP);
	clk_disable(gepf_clk.CG_MM_LARB5);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON_2X);
	clk_disable(gepf_clk.CG_MM_SMI_COMMON);
	clk_disable(gepf_clk.CG_SCP_SYS_MM0);
}

static inline void GEPF_Disable_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: GEPF clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0 */
	clk_disable_unprepare(gepf_clk.CG_IMGSYS_GEPF);
#ifndef SMI_CLK
	clk_disable_unprepare(gepf_clk.CG_IMGSYS_LARB);
	clk_disable_unprepare(gepf_clk.CG_SCP_SYS_ISP);
	clk_disable_unprepare(gepf_clk.CG_MM_LARB5);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON_2X);
	clk_disable_unprepare(gepf_clk.CG_MM_SMI_COMMON);
	clk_disable_unprepare(gepf_clk.CG_SCP_SYS_MM0);
#else
	smi_bus_disable(SMI_LARB_IMGSYS1, "camera_gepf");
#endif
}
#endif

/*******************************************************************************
*
********************************************************************************/
static void GEPF_EnableClock(MBOOL En)
{
#if defined(EP_NO_CLKMGR)
	MUINT32 setReg;
#endif

	if (En) {		/* Enable clock. */
		/* LOG_DBG("Dpe clock enbled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		switch (g_u4EnableClockCount) {
		case 0:
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#ifndef EP_NO_CLKMGR
			GEPF_Prepare_Enable_ccf_clock();
#else
			/* Enable clock by hardcode:
			* 1. CAMSYS_CG_CLR (0x1A000008) = 0xffffffff;
			* 2. IMG_CG_CLR (0x15000008) = 0xffffffff;
			*/
			setReg = 0xFFFFFFFF;
			/* GEPF_WR32(CAMSYS_REG_CG_CLR, setReg); */
			/* GEPF_WR32(IMGSYS_REG_CG_CLR, setReg); */
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
#endif				/* #if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
			break;
		default:
			break;
		}
		spin_lock(&(GEPFInfo.SpinLockGEPF));
		g_u4EnableClockCount++;
		spin_unlock(&(GEPFInfo.SpinLockGEPF));
	} else {		/* Disable clock. */

		/* LOG_DBG("Dpe clock disabled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		spin_lock(&(GEPFInfo.SpinLockGEPF));
		g_u4EnableClockCount--;
		spin_unlock(&(GEPFInfo.SpinLockGEPF));
		switch (g_u4EnableClockCount) {
		case 0:
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#ifndef EP_NO_CLKMGR
			GEPF_Disable_Unprepare_ccf_clock();
#else
			/* Disable clock by hardcode:
			* 1. CAMSYS_CG_SET (0x1A000004) = 0xffffffff;
			* 2. IMG_CG_SET (0x15000004) = 0xffffffff;
			*/
			setReg = 0xFFFFFFFF;
			/* GEPF_WR32(CAMSYS_REG_CG_SET, setReg); */
			/* GEPF_WR32(IMGSYS_REG_CG_SET, setReg); */
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
#endif				/* #if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) */
			break;
		default:
			break;
		}
	}
}

/*******************************************************************************
*
********************************************************************************/
static inline void GEPF_Reset(void)
{
	LOG_DBG("- E.");

	LOG_DBG(" GEPF Reset start!\n");
	spin_lock(&(GEPFInfo.SpinLockGEPFRef));

	if (GEPFInfo.UserCount > 1) {
		spin_unlock(&(GEPFInfo.SpinLockGEPFRef));
		LOG_DBG("Curr UserCount(%d) users exist", GEPFInfo.UserCount);
	} else {
		spin_unlock(&(GEPFInfo.SpinLockGEPFRef));

		/* Reset GEPF flow */
		GEPF_WR32(GEPF_DMA_RST_REG, 0x2);
		while ((GEPF_RD32(GEPF_DMA_RST_REG) & 0x04) != 0x4)
			LOG_DBG("GEPF resetting...\n");

		GEPF_WR32(GEPF_DMA_RST_REG, 0x01);
		GEPF_WR32(GEPF_DMA_RST_REG, 0x00);
		/* GEPF_WR32(RSC_RST_REG, 0x0); */
		/* GEPF_WR32(RSC_START_REG, 0); */
		LOG_DBG(" GEPF Reset end!\n");
	}

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_ReadReg(GEPF_REG_IO_STRUCT *pRegIo)
{
	MUINT32 i;
	MINT32 Ret = 0;
	/*  */
	GEPF_REG_STRUCT reg;
	/* MUINT32* pData = (MUINT32*)pRegIo->Data; */
	GEPF_REG_STRUCT *pData = (GEPF_REG_STRUCT *) pRegIo->pData;

	for (i = 0; i < pRegIo->Count; i++) {
		if (get_user(reg.Addr, (MUINT32 *) &pData->Addr) != 0) {
			LOG_ERR("get_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		/*  */
		if ((ISP_GEPF_BASE + reg.Addr >= ISP_GEPF_BASE)
		    && (ISP_GEPF_BASE + reg.Addr < (ISP_GEPF_BASE + GEPF_REG_RANGE))) {
			reg.Val = GEPF_RD32(ISP_GEPF_BASE + reg.Addr);
		} else {
			LOG_ERR("Wrong address(0x%p)", (ISP_GEPF_BASE + reg.Addr));
			reg.Val = 0;
		}
		/*  */
		/* printk("[KernelRDReg]addr(0x%x),value()0x%x\n",GEPF_ADDR_CAMINF + reg.Addr,reg.Val); */

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
static MINT32 GEPF_WriteRegToHw(GEPF_REG_STRUCT *pReg, MUINT32 Count)
{
	MINT32 Ret = 0;
	MUINT32 i;
	MBOOL dbgWriteReg;

	/* Use local variable to store GEPFInfo.DebugMask & GEPF_DBG_WRITE_REG for saving lock time */
	spin_lock(&(GEPFInfo.SpinLockGEPF));
	dbgWriteReg = GEPFInfo.DebugMask & GEPF_DBG_WRITE_REG;
	spin_unlock(&(GEPFInfo.SpinLockGEPF));

	/*  */
	if (dbgWriteReg)
		LOG_DBG("- E.\n");

	/*  */
	for (i = 0; i < Count; i++) {
		if (dbgWriteReg) {
			LOG_DBG("Addr(0x%lx), Val(0x%x)\n",
				(unsigned long)(ISP_GEPF_BASE + pReg[i].Addr),
				(MUINT32) (pReg[i].Val));
		}

		if (((ISP_GEPF_BASE + pReg[i].Addr) < (ISP_GEPF_BASE + GEPF_REG_RANGE))) {
			GEPF_WR32(ISP_GEPF_BASE + pReg[i].Addr, pReg[i].Val);
		} else {
			LOG_ERR("wrong address(0x%lx)\n",
				(unsigned long)(ISP_GEPF_BASE + pReg[i].Addr));
		}
	}

	/*  */
	return Ret;
}



/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_WriteReg(GEPF_REG_IO_STRUCT *pRegIo)
{
	MINT32 Ret = 0;
	/*
	* MINT32 TimeVd = 0;
	* MINT32 TimeExpdone = 0;
	* MINT32 TimeTasklet = 0;
	*/
	/* MUINT8* pData = NULL; */
	GEPF_REG_STRUCT *pData = NULL;
	/*  */
	if (GEPFInfo.DebugMask & GEPF_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n", (pRegIo->pData), (pRegIo->Count));

	/* pData = (MUINT8*)kmalloc((pRegIo->Count)*sizeof(GEPF_REG_STRUCT), GFP_ATOMIC); */
	pData = kmalloc((pRegIo->Count) * sizeof(GEPF_REG_STRUCT), GFP_ATOMIC);
	if (pData == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)\n", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	}
	/*  */
	if (copy_from_user
	    (pData, (void __user *)(pRegIo->pData), pRegIo->Count * sizeof(GEPF_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = GEPF_WriteRegToHw(pData, pRegIo->Count);
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
static MINT32 GEPF_WaitIrq(GEPF_WAIT_IRQ_STRUCT *WaitIrq)
{

	MINT32 Ret = 0;
	MINT32 Timeout = WaitIrq->Timeout;
	GEPF_PROCESS_ID_ENUM whichReq = GEPF_PROCESS_ID_NONE;

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
	if (GEPFInfo.DebugMask & GEPF_DBG_INT) {
		if (WaitIrq->Status & GEPFInfo.IrqInfo.Mask[WaitIrq->Type]) {
			if (WaitIrq->UserKey > 0) {
				LOG_DBG("+WaitIrqClr(%d),Type(%d),Status(0x%08X),Timeout(%d),user(%d),ProcID(%d)\n",
					 WaitIrq->Clear, WaitIrq->Type, WaitIrq->Status,
				     WaitIrq->Timeout, WaitIrq->UserKey, WaitIrq->ProcessID);
			}
		}
	}


	/* 1. wait type update */
	if (WaitIrq->Clear == GEPF_IRQ_CLEAR_STATUS) {
		spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		/* LOG_DBG("WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been cleared"
		* ,WaitIrq->EventInfo.Clear,WaitIrq->Type,GEPFInfo.IrqInfo.Status[WaitIrq->Type]);
		*/
		/* GEPFInfo.IrqInfo.Status[WaitIrq->Type][WaitIrq->EventInfo.UserKey] &=
		* (~WaitIrq->EventInfo.Status);
		*/
		GEPFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
		spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		return Ret;
	}

	if (WaitIrq->Clear == GEPF_IRQ_CLEAR_WAIT) {
		spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		if (GEPFInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
			GEPFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);

		spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
	} else if (WaitIrq->Clear == GEPF_IRQ_CLEAR_ALL) {
		spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);

		GEPFInfo.IrqInfo.Status[WaitIrq->Type] = 0;
		spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
	}
	/* GEPF_IRQ_WAIT_CLEAR ==> do nothing */


	/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
	spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
	irqStatus = GEPFInfo.IrqInfo.Status[WaitIrq->Type];
	spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);

	if (WaitIrq->Status & GEPF_INT_ST) {
		whichReq = GEPF_PROCESS_ID_GEPF;
	} else {
		LOG_ERR("No Such Stats can be waited!! irq Type/User/Sts/Pid(0x%x/%d/0x%x/%d)\n",
			WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, WaitIrq->ProcessID);
	}

#ifdef GEPF_WAITIRQ_LOG
	LOG_INF("before wait_event: WaitIrq Timeout(%d) Clear(%d), Type(%d), IrqStatus(0x%08X)\n",
		 WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus);
	LOG_INF("before wait_event: WaitStatus(0x%08X), Timeout(%d),userKey(%d),whichReq(%d),ProcessID(%d)\n",
		 WaitIrq->Status, WaitIrq->Timeout, WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
	LOG_INF("before wait_event: GepfIrqCnt(0x%08X), WriteReqIdx(0x%08X), ReadReqIdx(0x%08X)\n",
		 GEPFInfo.IrqInfo.GepfIrqCnt, GEPFInfo.WriteReqIdx, GEPFInfo.ReadReqIdx);
#endif

	/* 2. start to wait signal */
	Timeout = wait_event_interruptible_timeout(GEPFInfo.WaitQueueHead,
						   GEPF_GetIRQState(WaitIrq->Type, WaitIrq->UserKey,
								   WaitIrq->Status, whichReq,
								   WaitIrq->ProcessID),
						   GEPF_MsToJiffies(WaitIrq->Timeout));

	/* check if user is interrupted by system signal */
	if ((Timeout != 0)
	    &&
	    (!GEPF_GetIRQState
	     (WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, whichReq, WaitIrq->ProcessID))) {
		LOG_DBG("int by sys signal, rtn val(%d), irq Type/User/Sts/whichReq/Pid(0x%x/%d/0x%x/%d/%d)\n",
		     Timeout, WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, whichReq, WaitIrq->ProcessID);
		Ret = -ERESTARTSYS;	/* actually it should be -ERESTARTSYS */
		goto EXIT;
	}
	/* timeout */
	if (Timeout == 0) {
		/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
		spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = GEPFInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);

		LOG_ERR("Timeout!!! ERRRR WaitIrq Timeout(%d) Clear(%d), Type(%d), IrqStatus(0x%08X)\n",
			 WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus);
		LOG_ERR("Timeout!!! ERRRR WaitStatus(0x%08X), Timeout(%d),userKey(%d),whichReq(%d),ProcessID(%d)\n",
			 WaitIrq->Status, WaitIrq->Timeout, WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
		LOG_ERR("Timeout!!! ERRRR GepfIrqCnt(0x%08X), WriteReqIdx(0x%08X), ReadReqIdx(0x%08X)\n",
		     GEPFInfo.IrqInfo.GepfIrqCnt, GEPFInfo.WriteReqIdx, GEPFInfo.ReadReqIdx);

		if (WaitIrq->bDumpReg)
			GEPF_DumpReg();

		Ret = -EFAULT;
		goto EXIT;
	} else {
		/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("GEPF_WaitIrq");
#endif

		spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = GEPFInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);

		if (WaitIrq->Clear == GEPF_IRQ_WAIT_CLEAR) {
			spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
#ifdef GEPF_USE_GCE

#ifdef GEPF_MULTIPROCESS_TIMEING_ISSUE
			GEPFInfo.ReadReqIdx =
			    (GEPFInfo.ReadReqIdx + 1) % _SUPPORT_MAX_GEPF_FRAME_REQUEST_;
			/* actually, it doesn't happen the timging issue!! */
			/* wake_up_interruptible(&GEPFInfo.WaitQueueHead); */
#endif
			if (WaitIrq->Status & GEPF_INT_ST) {
				GEPFInfo.IrqInfo.GepfIrqCnt--;
				if (GEPFInfo.IrqInfo.GepfIrqCnt == 0)
					GEPFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
			} else {
				LOG_ERR("GEPF_IRQ_WAIT_CLEAR Error, Type(%d), WaitStatus(0x%08X)",
					WaitIrq->Type, WaitIrq->Status);
			}
#else
			if (GEPFInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
				GEPFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
#endif
			spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		}

#ifdef GEPF_WAITIRQ_LOG
		LOG_INF("no Timeout!!!: WaitIrq Timeout(%d) Clear(%d), Type(%d), IrqStatus(0x%08X)\n",
			 WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus);
		LOG_INF("no Timeout!!!: WaitStatus(0x%08X), Timeout(%d),userKey(%d),whichReq(%d),ProcessID(%d)\n",
			 WaitIrq->Status, WaitIrq->Timeout, WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
		LOG_INF("no Timeout!!!: GepfIrqCnt(0x%08X), WriteReqIdx(0x%08X), ReadReqIdx(0x%08X)\n",
			 GEPFInfo.IrqInfo.GepfIrqCnt, GEPFInfo.WriteReqIdx, GEPFInfo.ReadReqIdx);
#endif

#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif

	}


EXIT:


	return Ret;
}


/*******************************************************************************
*
********************************************************************************/
static long GEPF_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	MINT32 Ret = 0;

	/*MUINT32 pid = 0;*/
	GEPF_REG_IO_STRUCT RegIo;
	GEPF_WAIT_IRQ_STRUCT IrqInfo;
	GEPF_CLEAR_IRQ_STRUCT ClearIrq;
	GEPF_Config gepf_GepfConfig;
	GEPF_Request gepf_GepfReq;
	MINT32 GepfWriteIdx = 0;
	int idx;
	GEPF_USER_INFO_STRUCT *pUserInfo;
	int enqueNum;
	int dequeNum;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */



	/*  */
	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		return -EFAULT;
	}
	/*  */
	pUserInfo = (GEPF_USER_INFO_STRUCT *) (pFile->private_data);
	/*  */
	switch (Cmd) {
	case GEPF_RESET:
		{
			spin_lock(&(GEPFInfo.SpinLockGEPF));
			GEPF_Reset();
			spin_unlock(&(GEPFInfo.SpinLockGEPF));
			break;
		}

		/*  */
	case GEPF_DUMP_REG:
		{
			Ret = GEPF_DumpReg();
			break;
		}
	case GEPF_DUMP_ISR_LOG:
		{
			MUINT32 currentPPB = m_CurrentPPB;

			spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
			m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
			spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
					       flags);

			IRQ_LOG_PRINTER(GEPF_IRQ_TYPE_INT_GEPF_ST, currentPPB, _LOG_INF);
			IRQ_LOG_PRINTER(GEPF_IRQ_TYPE_INT_GEPF_ST, currentPPB, _LOG_ERR);
			break;
		}
	case GEPF_READ_REGISTER:
		{
			if (copy_from_user(&RegIo, (void *)Param, sizeof(GEPF_REG_IO_STRUCT)) == 0) {
				/* 2nd layer behavoir of copy from user is implemented in GEPF_ReadReg(...) */
				Ret = GEPF_ReadReg(&RegIo);
			} else {
				LOG_ERR("GEPF_READ_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case GEPF_WRITE_REGISTER:
		{
			if (copy_from_user(&RegIo, (void *)Param, sizeof(GEPF_REG_IO_STRUCT)) == 0) {
				/* 2nd layer behavoir of copy from user is implemented in GEPF_WriteReg(...) */
				Ret = GEPF_WriteReg(&RegIo);
			} else {
				LOG_ERR("GEPF_WRITE_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case GEPF_WAIT_IRQ:
		{
			if (copy_from_user(&IrqInfo, (void *)Param, sizeof(GEPF_WAIT_IRQ_STRUCT)) ==
			    0) {
				/*  */
				if ((IrqInfo.Type >= GEPF_IRQ_TYPE_AMOUNT) || (IrqInfo.Type < 0)) {
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
				Ret = GEPF_WaitIrq(&IrqInfo);

				if (copy_to_user
				    ((void *)Param, &IrqInfo, sizeof(GEPF_WAIT_IRQ_STRUCT)) != 0) {
					LOG_ERR("copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("GEPF_WAIT_IRQ copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case GEPF_CLEAR_IRQ:
		{
			if (copy_from_user(&ClearIrq, (void *)Param, sizeof(GEPF_CLEAR_IRQ_STRUCT))
			    == 0) {
				LOG_DBG("GEPF_CLEAR_IRQ Type(%d)", ClearIrq.Type);

				if ((ClearIrq.Type >= GEPF_IRQ_TYPE_AMOUNT) || (ClearIrq.Type < 0)) {
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

				LOG_DBG("GEPF_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)\n",
					ClearIrq.Type, ClearIrq.Status,
					GEPFInfo.IrqInfo.Status[ClearIrq.Type]);
				spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[ClearIrq.Type]), flags);
				GEPFInfo.IrqInfo.Status[ClearIrq.Type] &= (~ClearIrq.Status);
				spin_unlock_irqrestore(&(GEPFInfo.SpinLockIrq[ClearIrq.Type]),
						       flags);
			} else {
				LOG_ERR("GEPF_CLEAR_IRQ copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case GEPF_ENQNUE_NUM:
		{
			/* enqueNum */
			if (copy_from_user(&enqueNum, (void *)Param, sizeof(int)) == 0) {
				if (GEPF_REQUEST_STATE_EMPTY ==
				    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
				    State) {
					spin_lock_irqsave(&
							  (GEPFInfo.
							   SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
							  flags);
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].processID =
					    pUserInfo->Pid;
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].enqueReqNum =
					    enqueNum;
					spin_unlock_irqrestore(&
							       (GEPFInfo.
								SpinLockIrq
								[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
					if (enqueNum > _SUPPORT_MAX_GEPF_FRAME_REQUEST_) {
						LOG_ERR
						    ("GEPF Enque Num is bigger than enqueNum:%d\n",
						     enqueNum);
					}
					LOG_DBG("GEPF_ENQNUE_NUM:%d\n", enqueNum);
				} else {
					LOG_ERR
					    ("GEPF Enque request state is not empty:%d, writeIdx:%d, readIdx:%d\n",
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									       WriteIdx].
					     State, g_GEPF_ReqRing.WriteIdx,
					     g_GEPF_ReqRing.ReadIdx);
				}
			} else {
				LOG_ERR("GEPF_GEPF_EQNUE_NUM copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
		/* GEPF_Config */
	case GEPF_ENQUE:
		{
			if (copy_from_user(&gepf_GepfConfig, (void *)Param, sizeof(GEPF_Config))
			    == 0) {
				/* LOG_DBG("GEPF_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)",
				* ClearIrq.Type, ClearIrq.Status, GEPFInfo.IrqInfo.Status[ClearIrq.Type]);
				*/
				spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						  flags);
				if ((GEPF_REQUEST_STATE_EMPTY ==
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
				     State)
				    && (g_GEPF_ReqRing.
					GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].FrameWRIdx <
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].enqueReqNum)) {
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].
					    GepfFrameStatus[g_GEPF_ReqRing.
							    GEPFReq_Struct[g_GEPF_ReqRing.
									   WriteIdx].FrameWRIdx] =
					    GEPF_FRAME_STATUS_ENQUE;
					memcpy(&g_GEPF_ReqRing.
					       GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
					       GepfFrameConfig[g_GEPF_ReqRing.
							       GEPFReq_Struct[g_GEPF_ReqRing.
									      WriteIdx].
							       FrameWRIdx++], &gepf_GepfConfig,
					       sizeof(GEPF_Config));
					if (g_GEPF_ReqRing.
					    GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
					    FrameWRIdx ==
					    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									      WriteIdx].
					    enqueReqNum) {
						g_GEPF_ReqRing.
						    GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
						    State = GEPF_REQUEST_STATE_PENDING;
						g_GEPF_ReqRing.WriteIdx =
						    (g_GEPF_ReqRing.WriteIdx +
						     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
						LOG_DBG("GEPF enque done!!\n");
					} else {
						LOG_DBG("GEPF enque frame!!\n");
					}
				} else {
					LOG_ERR("NoEmptyGEPFBuf! WriteIdx(%d),State(%d),FraWRIdx(%d),enqReqNum(%d)\n",
					     g_GEPF_ReqRing.WriteIdx,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].State,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].FrameWRIdx,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].enqueReqNum);
				}
#ifdef GEPF_USE_GCE
				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);
				LOG_DBG("ConfigGEPF!!\n");
				ConfigGEPF();
#else
				/* check the hw is running or not ? */
				if (Check_GEPF_Is_Busy() == MFALSE) {
					/* config the gepf hw and run */
					LOG_DBG("ConfigGEPF\n");
					ConfigGEPF();
				} else {
					LOG_INF("GEPF HW is busy!!\n");
				}
				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);
#endif


			} else {
				LOG_ERR("GEPF_GEPF_ENQUE copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	case GEPF_ENQUE_REQ:
		{
			if (copy_from_user(&gepf_GepfReq, (void *)Param, sizeof(GEPF_Request)) ==
			    0) {
				LOG_DBG("GEPF_ENQNUE_NUM:%d, pid:%d\n", gepf_GepfReq.m_ReqNum,
					pUserInfo->Pid);
				if (gepf_GepfReq.m_ReqNum > _SUPPORT_MAX_GEPF_FRAME_REQUEST_) {
					LOG_ERR("GEPF Enque Num is bigger than enqueNum:%d\n",
						gepf_GepfReq.m_ReqNum);
					Ret = -EFAULT;
					goto EXIT;
				}
				if (copy_from_user
				    (g_GepfEnqueReq_Struct.GepfFrameConfig,
				     (void *)gepf_GepfReq.m_pGepfConfig,
				     gepf_GepfReq.m_ReqNum * sizeof(GEPF_Config)) != 0) {
					LOG_ERR("copy GEPFConfig from request is fail!!\n");
					Ret = -EFAULT;
					goto EXIT;
				}

				mutex_lock(&gGepfMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						  flags);
				if (GEPF_REQUEST_STATE_EMPTY ==
				    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
				    State) {
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].processID =
					    pUserInfo->Pid;
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].enqueReqNum =
					    gepf_GepfReq.m_ReqNum;

					for (idx = 0; idx < gepf_GepfReq.m_ReqNum; idx++) {
						g_GEPF_ReqRing.
						    GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
						    GepfFrameStatus[g_GEPF_ReqRing.
								    GEPFReq_Struct
								    [g_GEPF_ReqRing.WriteIdx].
								    FrameWRIdx] =
						    GEPF_FRAME_STATUS_ENQUE;
						memcpy(&g_GEPF_ReqRing.
						       GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].
						       GepfFrameConfig[g_GEPF_ReqRing.
								       GEPFReq_Struct
								       [g_GEPF_ReqRing.
									WriteIdx].FrameWRIdx++],
						       &g_GepfEnqueReq_Struct.GepfFrameConfig[idx],
						       sizeof(GEPF_Config));
					}
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  WriteIdx].State =
					    GEPF_REQUEST_STATE_PENDING;
					GepfWriteIdx = g_GEPF_ReqRing.WriteIdx;
					g_GEPF_ReqRing.WriteIdx =
					    (g_GEPF_ReqRing.WriteIdx +
					     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
					LOG_DBG("GEPF request enque done!!\n");
				} else {
					LOG_ERR("NoEmptyGEPFBuf! WriteIdx(%d),State(%d),FraWRIdx(%d),enqReqNum(%d)\n",
					     g_GEPF_ReqRing.WriteIdx,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].State,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].FrameWRIdx,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.WriteIdx].enqueReqNum);
				}
				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);
				LOG_DBG("ConfigGEPF Request!!\n");
				ConfigGEPFRequest(GepfWriteIdx);

				mutex_unlock(&gGepfMutex);
			} else {
				LOG_ERR("GEPF_ENQUE_REQ copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	case GEPF_DEQUE_NUM:
		{
			if (GEPF_REQUEST_STATE_FINISHED ==
			    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
			    State) {
				dequeNum =
				    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    enqueReqNum;
				LOG_DBG("GEPF_DEQUE_NUM(%d)\n", dequeNum);
			} else {
				dequeNum = 0;
				LOG_ERR("DeqNum:NoAnyGEPFReadyBuf!!ReadIdx(%d),State(%d),FraRDIdx(%d),enqReqNum(%d)\n",
				     g_GEPF_ReqRing.ReadIdx,
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].State,
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].FrameRDIdx,
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].enqueReqNum);
			}
			if (copy_to_user((void *)Param, &dequeNum, sizeof(MUINT32)) != 0) {
				LOG_ERR("GEPF_GEPF_DEQUE_NUM copy_to_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}

	case GEPF_DEQUE:
		{
			spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]), flags);
			if ((GEPF_REQUEST_STATE_FINISHED ==
			     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
			     State)
			    && (g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				FrameRDIdx <
				g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				enqueReqNum)) {
				/* dequeNum = g_DVE_RequestRing.DVEReq_Struct[g_DVE_RequestRing.ReadIdx].enqueReqNum; */
				if (GEPF_FRAME_STATUS_FINISHED ==
				    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    GepfFrameStatus[g_GEPF_ReqRing.
						    GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
						    FrameRDIdx]) {

					memcpy(&gepf_GepfConfig,
					       &g_GEPF_ReqRing.
					       GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
					       GepfFrameConfig[g_GEPF_ReqRing.
							       GEPFReq_Struct[g_GEPF_ReqRing.
									      ReadIdx].FrameRDIdx],
					       sizeof(GEPF_Config));
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  ReadIdx].
					    GepfFrameStatus[g_GEPF_ReqRing.
							    GEPFReq_Struct[g_GEPF_ReqRing.
									   ReadIdx].FrameRDIdx++] =
					    GEPF_FRAME_STATUS_EMPTY;
				}
				if (g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    FrameRDIdx ==
				    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    enqueReqNum) {
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  ReadIdx].State =
					    GEPF_REQUEST_STATE_EMPTY;
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  ReadIdx].FrameWRIdx = 0;
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  ReadIdx].FrameRDIdx = 0;
					g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									  ReadIdx].enqueReqNum = 0;
					g_GEPF_ReqRing.ReadIdx =
					    (g_GEPF_ReqRing.ReadIdx +
					     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
					LOG_DBG("GEPF ReadIdx(%d)\n", g_GEPF_ReqRing.ReadIdx);
				}
				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);
				if (copy_to_user
				    ((void *)Param,
				     &g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				     GepfFrameConfig[g_GEPF_ReqRing.
						     GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
						     FrameRDIdx], sizeof(GEPF_Config)) != 0) {
					LOG_ERR("GEPF_GEPF_DEQUE copy_to_user failed\n");
					Ret = -EFAULT;
				}

			} else {
				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);
				LOG_ERR("No Any GEPF Ready Buf!!, ReadIdx(%d), State(%d), FraRDIdx(%d), enqNum(%d)\n",
				     g_GEPF_ReqRing.ReadIdx,
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].State,
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].FrameRDIdx,
				     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].enqueReqNum);
			}

			break;
		}
	case GEPF_DEQUE_REQ:
		{
			if (copy_from_user(&gepf_GepfReq, (void *)Param, sizeof(GEPF_Request)) ==
			    0) {
				mutex_lock(&gGepfDequeMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						  flags);

				if (GEPF_REQUEST_STATE_FINISHED ==
				    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    State) {
					dequeNum =
					    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									      ReadIdx].enqueReqNum;
					LOG_DBG("GEPF_DEQUE_NUM(%d)\n", dequeNum);
				} else {
					dequeNum = 0;
					LOG_ERR("DeqNum:NoAnyRdyBuf!!RdIdx(%d),Sta(%d),FraRDIdx(%d),enqReqNum(%d)\n",
					     g_GEPF_ReqRing.ReadIdx,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].State,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].FrameRDIdx,
					     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].enqueReqNum);
				}
				gepf_GepfReq.m_ReqNum = dequeNum;

				for (idx = 0; idx < dequeNum; idx++) {
					if (GEPF_FRAME_STATUS_FINISHED ==
					    g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
									      ReadIdx].
					    GepfFrameStatus[g_GEPF_ReqRing.
							    GEPFReq_Struct[g_GEPF_ReqRing.
									   ReadIdx].FrameRDIdx]) {

						memcpy(&g_GepfDequeReq_Struct.GepfFrameConfig[idx],
						       &g_GEPF_ReqRing.
						       GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
						       GepfFrameConfig[g_GEPF_ReqRing.
								       GEPFReq_Struct
								       [g_GEPF_ReqRing.ReadIdx].
								       FrameRDIdx],
						       sizeof(GEPF_Config));
						g_GEPF_ReqRing.
						    GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
						    GepfFrameStatus[g_GEPF_ReqRing.
								    GEPFReq_Struct
								    [g_GEPF_ReqRing.ReadIdx].
								    FrameRDIdx++] =
						    GEPF_FRAME_STATUS_EMPTY;
					} else {
						LOG_ERR("Err!id(%d),deqNum(%d),RdId(%d),FraRDId(%d),GepfFraSta(%d)\n",
						     idx, dequeNum, g_GEPF_ReqRing.ReadIdx,
						     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].FrameRDIdx,
						     g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
						     GepfFrameStatus[g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.
						     ReadIdx].FrameRDIdx]);
					}
				}
				g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    State = GEPF_REQUEST_STATE_EMPTY;
				g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    FrameWRIdx = 0;
				g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    FrameRDIdx = 0;
				g_GEPF_ReqRing.GEPFReq_Struct[g_GEPF_ReqRing.ReadIdx].
				    enqueReqNum = 0;
				g_GEPF_ReqRing.ReadIdx =
				    (g_GEPF_ReqRing.ReadIdx +
				     1) % _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_;
				LOG_DBG("GEPF Request ReadIdx(%d)\n", g_GEPF_ReqRing.ReadIdx);

				spin_unlock_irqrestore(&
						       (GEPFInfo.
							SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]),
						       flags);

				mutex_unlock(&gGepfDequeMutex);

				if (copy_to_user
				    ((void *)gepf_GepfReq.m_pGepfConfig,
				     &g_GepfDequeReq_Struct.GepfFrameConfig[0],
				     dequeNum * sizeof(GEPF_Config)) != 0) {
					LOG_ERR
					    ("GEPF_GEPF_DEQUE_REQ copy_to_user frameconfig failed\n");
					Ret = -EFAULT;
				}
				if (copy_to_user
				    ((void *)Param, &gepf_GepfReq, sizeof(GEPF_Request)) != 0) {
					LOG_ERR("GEPF_GEPF_DEQUE_REQ copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("GEPF_CMD_GEPF_DEQUE_REQ copy_from_user failed\n");
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
static int compat_get_GEPF_read_register_data(compat_GEPF_REG_IO_STRUCT __user *data32,
					     GEPF_REG_IO_STRUCT __user *data)
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

static int compat_put_GEPF_read_register_data(compat_GEPF_REG_IO_STRUCT __user *data32,
					     GEPF_REG_IO_STRUCT __user *data)
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

static int compat_get_GEPF_enque_req_data(compat_GEPF_Request __user *data32,
					      GEPF_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pGepfConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pGepfConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_GEPF_enque_req_data(compat_GEPF_Request __user *data32,
					      GEPF_Request __user *data)
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


static int compat_get_GEPF_deque_req_data(compat_GEPF_Request __user *data32,
					      GEPF_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pGepfConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pGepfConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_GEPF_deque_req_data(compat_GEPF_Request __user *data32,
					      GEPF_Request __user *data)
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

static long GEPF_ioctl_compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;


	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		LOG_ERR("no f_op !!!\n");
		return -ENOTTY;
	}
	switch (cmd) {
	case COMPAT_GEPF_READ_REGISTER:
		{
			compat_GEPF_REG_IO_STRUCT __user *data32;
			GEPF_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_GEPF_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_get_GEPF_read_register_data error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, GEPF_READ_REGISTER,
						       (unsigned long)data);
			err = compat_put_GEPF_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_put_GEPF_read_register_data error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_GEPF_WRITE_REGISTER:
		{
			compat_GEPF_REG_IO_STRUCT __user *data32;
			GEPF_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_GEPF_read_register_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_GEPF_WRITE_REGISTER error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, GEPF_WRITE_REGISTER,
						       (unsigned long)data);
			return ret;
		}
	case COMPAT_GEPF_ENQUE_REQ:
		{
			compat_GEPF_Request __user *data32;
			GEPF_Request __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_GEPF_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_GEPF_GEPF_ENQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, GEPF_ENQUE_REQ,
						       (unsigned long)data);
			err = compat_put_GEPF_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_GEPF_GEPF_ENQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_GEPF_DEQUE_REQ:
		{
			compat_GEPF_Request __user *data32;
			GEPF_Request __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_GEPF_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_GEPF_GEPF_DEQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, GEPF_DEQUE_REQ,
						       (unsigned long)data);
			err = compat_put_GEPF_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_GEPF_GEPF_DEQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}

	case GEPF_WAIT_IRQ:
	case GEPF_CLEAR_IRQ:	/* structure (no pointer) */
	case GEPF_ENQNUE_NUM:
	case GEPF_ENQUE:
	case GEPF_DEQUE_NUM:
	case GEPF_DEQUE:
	case GEPF_RESET:
	case GEPF_DUMP_REG:
	case GEPF_DUMP_ISR_LOG:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return GEPF_ioctl(filep, cmd, arg); */
	}
}

#endif

/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_open(struct inode *pInode, struct file *pFile)
{
	MINT32 Ret = 0;
	MUINT32 i, j;
	/*int q = 0, p = 0;*/
	GEPF_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("- E. UserCount: %d.", GEPFInfo.UserCount);


	/*  */
	spin_lock(&(GEPFInfo.SpinLockGEPFRef));

	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(GEPF_USER_INFO_STRUCT), GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (GEPF_USER_INFO_STRUCT *) pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
	if (GEPFInfo.UserCount > 0) {
		GEPFInfo.UserCount++;
		spin_unlock(&(GEPFInfo.SpinLockGEPFRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			GEPFInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else {
		GEPFInfo.UserCount++;
		spin_unlock(&(GEPFInfo.SpinLockGEPFRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), first user",
			GEPFInfo.UserCount, current->comm, current->pid, current->tgid);
	}

	/* do wait queue head init when re-enter in camera */
	/*  */
	for (i = 0; i < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; i++) {
		/* GEPF */
		g_GEPF_ReqRing.GEPFReq_Struct[i].processID = 0x0;
		g_GEPF_ReqRing.GEPFReq_Struct[i].callerID = 0x0;
		g_GEPF_ReqRing.GEPFReq_Struct[i].enqueReqNum = 0x0;
		/* g_GEPF_ReqRing.GEPFReq_Struct[i].enqueIdx = 0x0; */
		g_GEPF_ReqRing.GEPFReq_Struct[i].State = GEPF_REQUEST_STATE_EMPTY;
		g_GEPF_ReqRing.GEPFReq_Struct[i].FrameWRIdx = 0x0;
		g_GEPF_ReqRing.GEPFReq_Struct[i].FrameRDIdx = 0x0;
		for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; j++) {
			g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j] =
			    GEPF_FRAME_STATUS_EMPTY;
		}

	}
	g_GEPF_ReqRing.WriteIdx = 0x0;
	g_GEPF_ReqRing.ReadIdx = 0x0;
	g_GEPF_ReqRing.HWProcessIdx = 0x0;

	/* Enable clock */
	GEPF_EnableClock(MTRUE);
	LOG_DBG("GEPF open g_u4EnableClockCount: %d", g_u4EnableClockCount);
	/*  */

	for (i = 0; i < GEPF_IRQ_TYPE_AMOUNT; i++)
		GEPFInfo.IrqInfo.Status[i] = 0;

	for (i = 0; i < _SUPPORT_MAX_GEPF_FRAME_REQUEST_; i++)
		GEPFInfo.ProcessID[i] = 0;

	GEPFInfo.WriteReqIdx = 0;
	GEPFInfo.ReadReqIdx = 0;
	GEPFInfo.IrqInfo.GepfIrqCnt = 0;

#define KERNEL_LOG
#ifdef KERNEL_LOG
    /* In EP, Add GEPF_DBG_WRITE_REG for debug. Should remove it after EP */
	GEPFInfo.DebugMask = (GEPF_DBG_INT | GEPF_DBG_DBGLOG | GEPF_DBG_WRITE_REG);
#endif
	/*  */
EXIT:




	LOG_DBG("- X. Ret: %d. UserCount: %d.", Ret, GEPFInfo.UserCount);
	return Ret;

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_release(struct inode *pInode, struct file *pFile)
{
	GEPF_USER_INFO_STRUCT *pUserInfo;
	/*MUINT32 Reg;*/

	LOG_DBG("- E. UserCount: %d.", GEPFInfo.UserCount);

	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo = (GEPF_USER_INFO_STRUCT *) pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	/*  */
	spin_lock(&(GEPFInfo.SpinLockGEPFRef));
	GEPFInfo.UserCount--;

	if (GEPFInfo.UserCount > 0) {
		spin_unlock(&(GEPFInfo.SpinLockGEPFRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			GEPFInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else
		spin_unlock(&(GEPFInfo.SpinLockGEPFRef));
	/*  */
	LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), last user",
		GEPFInfo.UserCount, current->comm, current->pid, current->tgid);


	/* Disable clock. */
	GEPF_EnableClock(MFALSE);
	LOG_DBG("GEPF release g_u4EnableClockCount: %d", g_u4EnableClockCount);

	/*  */
EXIT:


	LOG_DBG("- X. UserCount: %d.", GEPFInfo.UserCount);
	return 0;
}


/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	long length = 0;
	MUINT32 pfn = 0x0;

	length = pVma->vm_end - pVma->vm_start;
	/*  */
	pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
	pfn = pVma->vm_pgoff << PAGE_SHIFT;

	LOG_INF("GEPF_mmap:pVma->vm_pgoff(0x%lx),pfn(0x%x),phy(0x%lx)\n",
		 pVma->vm_pgoff, pfn, pVma->vm_pgoff << PAGE_SHIFT);
	LOG_INF("GEPF_mmap:pVmapVma->vm_start(0x%lx),pVma->vm_end(0x%lx),length(0x%lx)\n",
		 pVma->vm_start, pVma->vm_end, length);

	switch (pfn) {
	case GEPF_BASE_HW:
		if (length > GEPF_REG_RANGE) {
			LOG_ERR("mmap range error :module:0x%x length(0x%lx),GEPF_REG_RANGE(0x%x)!",
				pfn, length, GEPF_REG_RANGE);
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

static dev_t GEPFDevNo;
static struct cdev *pGEPFCharDrv;
static struct class *pGEPFClass;

static const struct file_operations GEPFFileOper = {
	.owner = THIS_MODULE,
	.open = GEPF_open,
	.release = GEPF_release,
	/* .flush   = mt_GEPF_flush, */
	.mmap = GEPF_mmap,
	.unlocked_ioctl = GEPF_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = GEPF_ioctl_compat,
#endif
};

/*******************************************************************************
*
********************************************************************************/
static inline void GEPF_UnregCharDev(void)
{
	LOG_DBG("- E.");
	/*  */
	/* Release char driver */
	if (pGEPFCharDrv != NULL) {
		cdev_del(pGEPFCharDrv);
		pGEPFCharDrv = NULL;
	}
	/*  */
	unregister_chrdev_region(GEPFDevNo, 1);
}

/*******************************************************************************
*
********************************************************************************/
static inline MINT32 GEPF_RegCharDev(void)
{
	MINT32 Ret = 0;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = alloc_chrdev_region(&GEPFDevNo, 0, 1, GEPF_DEV_NAME);
	if (Ret < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d", Ret);
		return Ret;
	}
	/* Allocate driver */
	pGEPFCharDrv = cdev_alloc();
	if (pGEPFCharDrv == NULL) {
		LOG_ERR("cdev_alloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/* Attatch file operation. */
	cdev_init(pGEPFCharDrv, &GEPFFileOper);
	/*  */
	pGEPFCharDrv->owner = THIS_MODULE;
	/* Add to system */
	Ret = cdev_add(pGEPFCharDrv, GEPFDevNo, 1);
	if (Ret < 0) {
		LOG_ERR("Attatch file operation failed, %d", Ret);
		goto EXIT;
	}
	/*  */
EXIT:
	if (Ret < 0)
		GEPF_UnregCharDev();

	/*  */

	LOG_DBG("- X.");
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_probe(struct platform_device *pDev)
{
	MINT32 Ret = 0;
	/*struct resource *pRes = NULL;*/
	MINT32 i = 0;
	MUINT8 n;
	MUINT32 irq_info[3];	/* Record interrupts info from device tree */
	struct device *dev = NULL;
	struct GEPF_device *_gepf_dev;

#ifdef CONFIG_OF
	struct GEPF_device *GEPF_dev;
#endif

	LOG_INF("- E. GEPF driver probe.");

	/* Check platform_device parameters */
#ifdef CONFIG_OF

	if (pDev == NULL) {
		dev_err(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}

	nr_GEPF_devs += 1;
	_gepf_dev = krealloc(GEPF_devs, sizeof(struct GEPF_device) * nr_GEPF_devs, GFP_KERNEL);
	if (!_gepf_dev) {
		dev_err(&pDev->dev, "Unable to allocate GEPF_devs\n");
		return -ENOMEM;
	}
	GEPF_devs = _gepf_dev;

	GEPF_dev = &(GEPF_devs[nr_GEPF_devs - 1]);
	GEPF_dev->dev = &pDev->dev;

	/* iomap registers */
	GEPF_dev->regs = of_iomap(pDev->dev.of_node, 0);
	/* gISPSYS_Reg[nr_GEPF_devs - 1] = GEPF_dev->regs; */

	if (!GEPF_dev->regs) {
		dev_err(&pDev->dev,
			"Unable to ioremap registers, of_iomap fail, nr_GEPF_devs=%d, devnode(%s).\n",
			nr_GEPF_devs, pDev->dev.of_node->name);
		return -ENOMEM;
	}

	LOG_INF("nr_GEPF_devs=%d, devnode(%s), map_addr=0x%lx\n", nr_GEPF_devs,
		pDev->dev.of_node->name, (unsigned long)GEPF_dev->regs);

	/* get IRQ ID and request IRQ */
	GEPF_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);

	if (GEPF_dev->irq > 0) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array
		    (pDev->dev.of_node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
			dev_err(&pDev->dev, "get irq flags from DTS fail!!\n");
			return -ENODEV;
		}

		for (i = 0; i < GEPF_IRQ_TYPE_AMOUNT; i++) {
			if (strcmp(pDev->dev.of_node->name, GEPF_IRQ_CB_TBL[i].device_name) == 0) {
				Ret =
				    request_irq(GEPF_dev->irq,
						(irq_handler_t) GEPF_IRQ_CB_TBL[i].isr_fp,
						irq_info[2],
						(const char *)GEPF_IRQ_CB_TBL[i].device_name, NULL);
				if (Ret) {
					dev_err(&pDev->dev,
						"Unable to request IRQ, request_irq fail, nr_GEPF_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
						nr_GEPF_devs, pDev->dev.of_node->name, GEPF_dev->irq,
						GEPF_IRQ_CB_TBL[i].device_name);
					return Ret;
				}

				LOG_INF("nr_GEPF_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
					nr_GEPF_devs, pDev->dev.of_node->name, GEPF_dev->irq,
					GEPF_IRQ_CB_TBL[i].device_name);
				break;
			}
		}

		if (i >= GEPF_IRQ_TYPE_AMOUNT) {
			LOG_INF("No corresponding ISR!!: nr_GEPF_devs=%d, devnode(%s), irq=%d\n",
				nr_GEPF_devs, pDev->dev.of_node->name, GEPF_dev->irq);
		}


	} else {
		LOG_INF("No IRQ!!: nr_GEPF_devs=%d, devnode(%s), irq=%d\n", nr_GEPF_devs,
			pDev->dev.of_node->name, GEPF_dev->irq);
	}


#endif

	/* Only register char driver in the 1st time */
	if (nr_GEPF_devs == 1) {

		/* Register char driver */
		Ret = GEPF_RegCharDev();
		if (Ret) {
			dev_err(&pDev->dev, "register char failed");
			return Ret;
		}
#ifndef EP_NO_CLKMGR
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
		    /*CCF: Grab clock pointer (struct clk*) */
		gepf_clk.CG_SCP_SYS_MM0 = devm_clk_get(&pDev->dev, "GEPF_SCP_SYS_MM0");
		gepf_clk.CG_MM_SMI_COMMON = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG2_B11");
		gepf_clk.CG_MM_SMI_COMMON_2X = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG2_B12");
		gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B12");
		gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B13");
		gepf_clk.CG_MM_SMI_COMMON_UPSZ0 = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B14");
		gepf_clk.CG_MM_SMI_COMMON_UPSZ1 = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B15");
		gepf_clk.CG_MM_SMI_COMMON_FIFO0 = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B16");
		gepf_clk.CG_MM_SMI_COMMON_FIFO1 = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B17");
		gepf_clk.CG_MM_LARB5 = devm_clk_get(&pDev->dev, "GEPF_CLK_MM_CG1_B10");
		gepf_clk.CG_SCP_SYS_ISP = devm_clk_get(&pDev->dev, "GEPF_SCP_SYS_ISP");
		gepf_clk.CG_IMGSYS_LARB = devm_clk_get(&pDev->dev, "GEPF_CLK_IMG_LARB");
		gepf_clk.CG_IMGSYS_GEPF = devm_clk_get(&pDev->dev, "GEPF_CLK_IMG_GEPF");

		if (IS_ERR(gepf_clk.CG_SCP_SYS_MM0)) {
			LOG_ERR("cannot get CG_SCP_SYS_MM0 clock\n");
			return PTR_ERR(gepf_clk.CG_SCP_SYS_MM0);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_2X clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_2X);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_GALS_M0_2X clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_GALS_M1_2X clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_UPSZ0)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_UPSZ0 clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_UPSZ0);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_UPSZ1)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_UPSZ1 clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_UPSZ1);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_FIFO0)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_FIFO0 clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_FIFO0);
		}
		if (IS_ERR(gepf_clk.CG_MM_SMI_COMMON_FIFO1)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_FIFO1 clock\n");
			return PTR_ERR(gepf_clk.CG_MM_SMI_COMMON_FIFO1);
		}
		if (IS_ERR(gepf_clk.CG_MM_LARB5)) {
			LOG_ERR("cannot get CG_MM_LARB5 clock\n");
			return PTR_ERR(gepf_clk.CG_MM_LARB5);
		}
		if (IS_ERR(gepf_clk.CG_SCP_SYS_ISP)) {
			LOG_ERR("cannot get CG_SCP_SYS_ISP clock\n");
			return PTR_ERR(gepf_clk.CG_SCP_SYS_ISP);
		}
		if (IS_ERR(gepf_clk.CG_IMGSYS_LARB)) {
			LOG_ERR("cannot get CG_IMGSYS_LARB clock\n");
			return PTR_ERR(gepf_clk.CG_IMGSYS_LARB);
		}
		if (IS_ERR(gepf_clk.CG_IMGSYS_GEPF)) {
			LOG_ERR("cannot get CG_IMGSYS_GEPF clock\n");
			return PTR_ERR(gepf_clk.CG_IMGSYS_GEPF);
		}
#endif				/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
#endif

		/* Create class register */
		pGEPFClass = class_create(THIS_MODULE, "GEPFdrv");
		if (IS_ERR(pGEPFClass)) {
			Ret = PTR_ERR(pGEPFClass);
			LOG_ERR("Unable to create class, err = %d", Ret);
			goto EXIT;
		}

		dev = device_create(pGEPFClass, NULL, GEPFDevNo, NULL, GEPF_DEV_NAME);
		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_err(&pDev->dev, "Failed to create device: /dev/%s, err = %d",
				GEPF_DEV_NAME, Ret);
			goto EXIT;
		}

		/* Init spinlocks */
		spin_lock_init(&(GEPFInfo.SpinLockGEPFRef));
		spin_lock_init(&(GEPFInfo.SpinLockGEPF));
		for (n = 0; n < GEPF_IRQ_TYPE_AMOUNT; n++)
			spin_lock_init(&(GEPFInfo.SpinLockIrq[n]));

		/*  */
		init_waitqueue_head(&GEPFInfo.WaitQueueHead);
		INIT_WORK(&GEPFInfo.ScheduleGepfWork, GEPF_ScheduleWork);

		wake_lock_init(&GEPF_wake_lock, WAKE_LOCK_SUSPEND, "gepf_lock_wakelock");

		for (i = 0; i < GEPF_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(GEPF_tasklet[i].pGEPF_tkt, GEPF_tasklet[i].tkt_cb, 0);




		/* Init GEPFInfo */
		GEPFInfo.UserCount = 0;
		/*  */
		GEPFInfo.IrqInfo.Mask[GEPF_IRQ_TYPE_INT_GEPF_ST] = INT_ST_MASK_GEPF;

	}

EXIT:
	if (Ret < 0)
		GEPF_UnregCharDev();


	LOG_INF("- X. GEPF driver probe.");

	return Ret;
}

/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static MINT32 GEPF_remove(struct platform_device *pDev)
{
	/*struct resource *pRes;*/
	MINT32 IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	/* unregister char driver. */
	GEPF_UnregCharDev();

	/* Release IRQ */
	disable_irq(GEPFInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* kill tasklet */
	for (i = 0; i < GEPF_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(GEPF_tasklet[i].pGEPF_tkt);
#if 0
	/* free all registered irq(child nodes) */
	GEPF_UnRegister_AllregIrq();
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
	device_destroy(pGEPFClass, GEPFDevNo);
	/*  */
	class_destroy(pGEPFClass);
	pGEPFClass = NULL;
	/*  */
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 bPass1_On_In_Resume_TG1;

static MINT32 GEPF_suspend(struct platform_device *pDev, pm_message_t Mesg)
{
	/*MINT32 ret = 0;*/

	LOG_DBG("bPass1_On_In_Resume_TG1(%d)\n", bPass1_On_In_Resume_TG1);

	bPass1_On_In_Resume_TG1 = 0;


	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 GEPF_resume(struct platform_device *pDev)
{
	LOG_DBG("bPass1_On_In_Resume_TG1(%d).\n", bPass1_On_In_Resume_TG1);

	return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int GEPF_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return GEPF_suspend(pdev, PMSG_SUSPEND);
}

int GEPF_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return GEPF_resume(pdev);
}
#ifndef CONFIG_OF
/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
#endif
int GEPF_pm_restore_noirq(struct device *device)
{
	pr_debug("calling %s()\n", __func__);
#ifndef CONFIG_OF
/*	mt_irq_set_sens(GEPF_IRQ_BIT_ID, MT_LEVEL_SENSITIVE); */
/*	mt_irq_set_polarity(GEPF_IRQ_BIT_ID, MT_POLARITY_LOW); */
#endif
	return 0;

}

/*---------------------------------------------------------------------------*/
#else				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#define GEPF_pm_suspend NULL
#define GEPF_pm_resume  NULL
#define GEPF_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_OF
/*
 * Note!!! The order and member of .compatible must be the same with that in
 *  "GEPF_DEV_NODE_ENUM" in camera_GEPF.h
 */
static const struct of_device_id GEPF_of_ids[] = {
	{.compatible = "mediatek,gepf",},
	{}
};
#endif

const struct dev_pm_ops GEPF_pm_ops = {
	.suspend = GEPF_pm_suspend,
	.resume = GEPF_pm_resume,
	.freeze = GEPF_pm_suspend,
	.thaw = GEPF_pm_resume,
	.poweroff = GEPF_pm_suspend,
	.restore = GEPF_pm_resume,
	.restore_noirq = GEPF_pm_restore_noirq,
};


/*******************************************************************************
*
********************************************************************************/
static struct platform_driver GEPFDriver = {
	.probe = GEPF_probe,
	.remove = GEPF_remove,
	.suspend = GEPF_suspend,
	.resume = GEPF_resume,
	.driver = {
		   .name = GEPF_DEV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = GEPF_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &GEPF_pm_ops,
#endif
		   }
};


static int gepf_dump_read(struct seq_file *m, void *v)
{
	int i, j;

	seq_puts(m, "\n============ gepf dump register============\n");
	seq_puts(m, "GEPF Config Info\n");

	for (i = 0x400; i < 0x4A8; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(GEPF_BASE_HW + i),
			   (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i));
	}
	seq_puts(m, "GEPF Debug Info\n");
	for (i = 0x4A8; i < 0x4B8; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(GEPF_BASE_HW + i),
			   (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i));
	}

	seq_puts(m, "\n");
	seq_printf(m, "Gepf Clock Count:%d\n", g_u4EnableClockCount);

	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DMA_DEBUG_DATA_HW),
		   (unsigned int)GEPF_RD32(DMA_DEBUG_DATA_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DMA_DCM_ST_HW),
		   (unsigned int)GEPF_RD32(DMA_DCM_ST_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DMA_RDY_ST_HW),
		   (unsigned int)GEPF_RD32(DMA_RDY_ST_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(DMA_REQ_ST_HW),
		   (unsigned int)GEPF_RD32(DMA_REQ_ST_REG));


	seq_printf(m, "GEPF:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		   g_GEPF_ReqRing.HWProcessIdx, g_GEPF_ReqRing.WriteIdx,
		   g_GEPF_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_GEPF_REQUEST_RING_SIZE_; i++) {
		seq_printf(m,
			   "GEPF:State:%d, processID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, FrameRDIdx:%d\n",
			   g_GEPF_ReqRing.GEPFReq_Struct[i].State,
			   g_GEPF_ReqRing.GEPFReq_Struct[i].processID,
			   g_GEPF_ReqRing.GEPFReq_Struct[i].callerID,
			   g_GEPF_ReqRing.GEPFReq_Struct[i].enqueReqNum,
			   g_GEPF_ReqRing.GEPFReq_Struct[i].FrameWRIdx,
			   g_GEPF_ReqRing.GEPFReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_GEPF_FRAME_REQUEST_;) {
			seq_printf(m,
				   "GEPF:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
				   j, g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j]
				   , j + 1,
				   g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j + 1],
				   j + 2,
				   g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j + 2]
				   , j + 3,
				   g_GEPF_ReqRing.GEPFReq_Struct[i].GepfFrameStatus[j + 3]);
			j = j + 4;
		}
	}

	seq_puts(m, "\n============ gepf dump debug ============\n");

	return 0;
}


static int proc_gepf_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, gepf_dump_read, NULL);
}

static const struct file_operations gepf_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_gepf_dump_open,
	.read = seq_read,
};


static int gepf_reg_read(struct seq_file *m, void *v)
{
	unsigned int i;

	seq_puts(m, "======== read gepf register ========\n");

	for (i = 0x400; i <= 0x4B4; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(GEPF_BASE_HW + i),
			   (unsigned int)GEPF_RD32(ISP_GEPF_BASE + i));
	}

	return 0;
}

/*static int gepf_reg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)*/

static ssize_t gepf_reg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	/*char *pEnd;*/
	char addrSzBuf[24];
	char valSzBuf[24];
	char *pszTmp;
	int addr, val;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%23s %23s", addrSzBuf, valSzBuf) == 2) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (sscanf(addrSzBuf, "%d", &addr) != 1)*/
			if (kstrtoint(addrSzBuf, 0, &addr) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s", addrSzBuf);
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("scan hexadecimal addr is wrong !!:%s", addrSzBuf);
			} else {
				LOG_INF("GEPF Write Addr Error!!:%s", addrSzBuf);
			}
		}

		pszTmp = strstr(valSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (sscanf(valSzBuf, "%d", &val) != 1)*/
			if (kstrtoint(valSzBuf, 0, &val) != 0)
				LOG_ERR("scan decimal value is wrong !!:%s", valSzBuf);
		} else {
			if (strlen(valSzBuf) > 2) {
				if (sscanf(valSzBuf + 2, "%x", &val) != 1)
					LOG_ERR("scan hexadecimal value is wrong !!:%s", valSzBuf);
			} else {
				LOG_INF("GEPF Write Value Error!!:%s\n", valSzBuf);
			}
		}

		if ((addr >= GEPF_BASE_HW) && (addr <= DMA_REQ_ST_HW)) {
			LOG_INF("Write Request - addr:0x%x, value:0x%x\n", addr, val);
			GEPF_WR32((ISP_GEPF_BASE + (addr - GEPF_BASE_HW)), val);
		} else {
			LOG_INF
			    ("Write-Address Range exceeds the size of hw gepf!! addr:0x%x, value:0x%x\n",
			     addr, val);
		}

	} else if (sscanf(desc, "%23s", addrSzBuf) == 1) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(addrSzBuf, "%d", &addr))*/
			if (kstrtoint(addrSzBuf, 0, &addr) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s", addrSzBuf);
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("scan hexadecimal addr is wrong !!:%s", addrSzBuf);
			} else {
				LOG_INF("GEPF Read Addr Error!!:%s", addrSzBuf);
			}
		}

		if ((addr >= GEPF_BASE_HW) && (addr <= DMA_REQ_ST_HW)) {
			val = GEPF_RD32((ISP_GEPF_BASE + (addr - GEPF_BASE_HW)));
			LOG_INF("Read Request - addr:0x%x,value:0x%x\n", addr, val);
		} else {
			LOG_INF
			    ("Read-Address Range exceeds the size of hw gepf!! addr:0x%x, value:0x%x\n",
			     addr, val);
		}

	}


	return count;
}

static int proc_gepf_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, gepf_reg_read, NULL);
}

static const struct file_operations gepf_reg_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_gepf_reg_open,
	.read = seq_read,
	.write = gepf_reg_write,
};


/*******************************************************************************
*
********************************************************************************/

int32_t GEPF_ClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("GEPF_ClockOnCallback"); */
	/* LOG_DBG("+CmdqEn:%d", g_u4EnableClockCount); */
	/* GEPF_EnableClock(MTRUE); */

	return 0;
}

int32_t GEPF_DumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("GEPF_DumpCallback");

	GEPF_DumpReg();

	return 0;
}

int32_t GEPF_ResetCallback(uint64_t engineFlag)
{
	LOG_DBG("GEPF_ResetCallback");
	GEPF_Reset();

	return 0;
}

int32_t GEPF_ClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("GEPF_ClockOffCallback"); */
	/* GEPF_EnableClock(MFALSE); */
	/* LOG_DBG("-CmdqEn:%d", g_u4EnableClockCount); */
	return 0;
}


static MINT32 __init GEPF_Init(void)
{
	MINT32 Ret = 0, j;
	void *tmp;
	/* FIX-ME: linux-3.10 procfs API changed */
	/* use proc_create */
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *isp_gepf_dir;


	int i;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = platform_driver_register(&GEPFDriver);
	if (Ret < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}

#if 0
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,GEPF");
	if (!node) {
		LOG_ERR("find mediatek,GEPF node failed!!!\n");
		return -ENODEV;
	}
	ISP_GEPF_BASE = of_iomap(node, 0);
	if (!ISP_GEPF_BASE) {
		LOG_ERR("unable to map ISP_GEPF_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_GEPF_BASE: %lx\n", ISP_GEPF_BASE);
#endif

	isp_gepf_dir = proc_mkdir("gepf", NULL);
	if (!isp_gepf_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/gepf\n", __func__);
		return 0;
	}

	/* proc_entry = proc_create("pll_test", S_IRUGO | S_IWUSR, isp_gepf_dir, &pll_test_proc_fops); */

	proc_entry = proc_create("gepf_dump", S_IRUGO, isp_gepf_dir, &gepf_dump_proc_fops);

	proc_entry = proc_create("gepf_reg", S_IRUGO | S_IWUSR, isp_gepf_dir, &gepf_reg_proc_fops);


	/* isr log */
	if (PAGE_SIZE <
	    ((GEPF_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN * ((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) *
	     LOG_PPNUM)) {
		i = 0;
		while (i <
		       ((GEPF_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
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
		for (j = 0; j < GEPF_IRQ_TYPE_AMOUNT; j++) {
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
	/* Register GEPF callback */
	LOG_DBG("register gepf callback for CMDQ");
	cmdqCoreRegisterCB(CMDQ_GROUP_GEPF,
			   GEPF_ClockOnCallback,
			   GEPF_DumpCallback, GEPF_ResetCallback, GEPF_ClockOffCallback);
#endif

	LOG_DBG("- X. Ret: %d.", Ret);
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static void __exit GEPF_Exit(void)
{
	/*int i;*/

	LOG_DBG("- E.");
	/*  */
	platform_driver_unregister(&GEPFDriver);
	/*  */
#if 1
	/* Cmdq */
	/* Unregister GEPF callback */
	cmdqCoreRegisterCB(CMDQ_GROUP_GEPF, NULL, NULL, NULL, NULL);
#endif

	kfree(pLog_kmalloc);

	/*  */
}


/*******************************************************************************
*
********************************************************************************/
void GEPF_ScheduleWork(struct work_struct *data)
{
	if (GEPF_DBG_DBGLOG & GEPFInfo.DebugMask)
		LOG_DBG("- E.");

#ifdef GEPF_USE_GCE
#else
	ConfigGEPF();
#endif
}


static irqreturn_t ISP_Irq_GEPF(MINT32 Irq, void *DeviceId)
{
	MUINT32 GepfStatus;
	MBOOL bResulst = MFALSE;
	pid_t ProcessID;

	GepfStatus = GEPF_RD32(GEPF_IRQ_REG);	/* GEPF Status */
	LOG_ERR("GEPF INTERRUPT Handler reads GEPF_INT_STATUS(%d)", GepfStatus);

	spin_lock(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]));

	if (0x0 == (GEPF_INT_ST & GepfStatus)) {
		/* Update the frame status. */
#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("gepf_irq");
#endif

#ifndef GEPF_USE_GCE
		GEPF_WR32(GEPF_CRT_0_REG, 0);	/* Daniel remove, need to check what for? */
		GEPF_WR32(GEPF_MODULE_DCM_REG, 0x0);    /* GEPF DCM Start Disable */
#endif
		/* GEPF_WR32(GEPF_IRQ_REG, 1);*/	/* For write 1 clear */
		bResulst = UpdateGEPF(&ProcessID);
		/* ConfigGEPF(); */
		if (bResulst == MTRUE) {
			schedule_work(&GEPFInfo.ScheduleGepfWork);
#ifdef GEPF_USE_GCE
			GEPFInfo.IrqInfo.Status[GEPF_IRQ_TYPE_INT_GEPF_ST] |= GEPF_INT_ST;
			GEPFInfo.IrqInfo.ProcessID[GEPF_PROCESS_ID_GEPF] = ProcessID;
			GEPFInfo.IrqInfo.GepfIrqCnt++;
			GEPFInfo.ProcessID[GEPFInfo.WriteReqIdx] = ProcessID;
			GEPFInfo.WriteReqIdx =
			    (GEPFInfo.WriteReqIdx + 1) % _SUPPORT_MAX_GEPF_FRAME_REQUEST_;
#ifdef GEPF_MULTIPROCESS_TIMEING_ISSUE
			/* check the write value is equal to read value ? */
			/* actually, it doesn't happen!! */
			if (GEPFInfo.WriteReqIdx == GEPFInfo.ReadReqIdx) {
				IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_ERR,
					       "ISP_Irq_GEPF Err!!, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
					       GEPFInfo.WriteReqIdx, GEPFInfo.ReadReqIdx);
			}
#endif

#else
			GEPFInfo.IrqInfo.Status[GEPF_IRQ_TYPE_INT_GEPF_ST] |= GEPF_INT_ST;
			GEPFInfo.IrqInfo.ProcessID[GEPF_PROCESS_ID_GEPF] = ProcessID;
#endif
		}
#ifdef __GEPF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}
	spin_unlock(&(GEPFInfo.SpinLockIrq[GEPF_IRQ_TYPE_INT_GEPF_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&GEPFInfo.WaitQueueHead);

	/* dump log, use tasklet */
	IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_INF,
		       "ISP_Irq_GEPF:%d, reg 0x%x : 0x%x, bResulst:%d, GepfHWSta:0x%x, GepfIrqCnt:0x%x, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
		       Irq, GEPF_IRQ_HW, GepfStatus, bResulst, GepfStatus,
		       GEPFInfo.IrqInfo.GepfIrqCnt,
		       GEPFInfo.WriteReqIdx, GEPFInfo.ReadReqIdx);
	/* IRQ_LOG_KEEPER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_INF, "DveHWSta:0x%x, GepfHWSta:0x%x,
	* DpeDveSta0:0x%x\n", DveStatus, GepfStatus, DpeDveSta0);
	*/

	if (GepfStatus & GEPF_INT_ST)
		tasklet_schedule(GEPF_tasklet[GEPF_IRQ_TYPE_INT_GEPF_ST].pGEPF_tkt);

	return IRQ_HANDLED;
}

static void ISP_TaskletFunc_GEPF(unsigned long data)
{
	IRQ_LOG_PRINTER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(GEPF_IRQ_TYPE_INT_GEPF_ST, m_CurrentPPB, _LOG_ERR);

}


/*******************************************************************************
*
********************************************************************************/
module_init(GEPF_Init);
module_exit(GEPF_Exit);
MODULE_DESCRIPTION("Camera GEPF driver");
MODULE_AUTHOR("MM3SW5");
MODULE_LICENSE("GPL");
