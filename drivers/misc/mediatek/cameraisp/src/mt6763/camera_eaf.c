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

/*  */
/*#include <mach/hardware.h>*/
/* #include <mach/mt6593_pll.h> */
#include "inc/camera_eaf.h"
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
  * #define __EAF_KERNEL_PERFORMANCE_MEASURE__
  */
#ifdef __EAF_KERNEL_PERFORMANCE_MEASURE__
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

/* EAF Command Queue */
/* #include "../../cmdq/mt6797/cmdq_record.h" */
/* #include "../../cmdq/mt6797/cmdq_core.h" */

/* CCF */
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#include <linux/clk.h>
struct EAF_CLK_STRUCT {
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
	struct clk *CG_IMGSYS_EAF;
};
struct EAF_CLK_STRUCT eaf_clk;
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

#define EAF_DEV_NAME                "camera-eaf"

/* #define EAF_WAITIRQ_LOG  */
#define EAF_USE_GCE
#define EAF_DEBUG_USE
#define SMI_CLK

/* #define EAF_MULTIPROCESS_TIMEING_ISSUE  */
/*I can' test the situation in FPGA, because the velocity of FPGA is so slow. */
#define MyTag "[EAF]"
#define IRQTag "KEEPER"

#define LOG_VRB(format,	args...)    pr_debug(MyTag format, ##args)

#ifdef EAF_DEBUG_USE
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
/* #define EAF_WR32(addr, data)    iowrite32(data, addr) // For other projects. */
#define EAF_WR32(addr, data)    mt_reg_sync_writel(data, addr)
#define EAF_RD32(addr)          ioread32((void *)addr)
#define EAF_SET_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) |= (MUINT32)(1 << (bit)))
#define EAF_CLR_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) &= ~((MUINT32)(1 << (bit))))
/*******************************************************************************
*
********************************************************************************/
/* dynamic log level */
#define EAF_DBG_DBGLOG              (0x00000001)
#define EAF_DBG_INFLOG              (0x00000002)
#define EAF_DBG_INT                 (0x00000004)
#define EAF_DBG_READ_REG            (0x00000008)
#define EAF_DBG_WRITE_REG           (0x00000010)
#define EAF_DBG_TASKLET             (0x00000020)


/* ///////////////////////////////////////////////////////////////// */

/*******************************************************************************
*
********************************************************************************/




/**
 * IRQ signal mask
*/

#define INT_ST_MASK_EAF     ( \
			EAF_INT_ST)

#define CMDQ_REG_MASK 0xffffffff

#define EAF_START      0x1

#define EAF_ENABLE     0x1

#define EAF_IS_BUSY    0x2



/* static irqreturn_t EAF_Irq_CAM_A(MINT32  Irq,void *DeviceId); */
static irqreturn_t ISP_Irq_EAF(MINT32 Irq, void *DeviceId);
static MBOOL ConfigEAF(void);
static MINT32 ConfigEAFHW(EAF_Config *pEafConfig);
static void EAF_ScheduleWork(struct work_struct *data);


typedef irqreturn_t(*IRQ_CB) (MINT32, void *);

typedef struct {
	IRQ_CB isr_fp;
	unsigned int int_number;
	char device_name[16];
} ISR_TABLE;

#ifndef CONFIG_OF
const ISR_TABLE EAF_IRQ_CB_TBL[EAF_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_EAF, EAF_IRQ_BIT_ID, "eaf"},
};

#else
/* int number is got from kernel api */
const ISR_TABLE EAF_IRQ_CB_TBL[EAF_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_EAF, 0, "eaf"},
};

#endif
/* //////////////////////////////////////////////////////////////////////////////////////////// */
/*  */
typedef void (*tasklet_cb) (unsigned long);
typedef struct {
	tasklet_cb tkt_cb;
	struct tasklet_struct *pEAF_tkt;
} Tasklet_table;

struct tasklet_struct EAFtkt[EAF_IRQ_TYPE_AMOUNT];

static void ISP_TaskletFunc_EAF(unsigned long data);

static Tasklet_table EAF_tasklet[EAF_IRQ_TYPE_AMOUNT] = {
	{ISP_TaskletFunc_EAF, &EAFtkt[EAF_IRQ_TYPE_INT_EAF_ST]},
};

struct wake_lock EAF_wake_lock;

static DEFINE_MUTEX(gEafMutex);
static DEFINE_MUTEX(gEafDequeMutex);

#ifdef CONFIG_OF

struct EAF_device {
	void __iomem *regs;
	struct device *dev;
	int irq;
};

static struct EAF_device *EAF_devs;
static int nr_EAF_devs;

/* Get HW modules' base address from device nodes */
#define EAF_DEV_NODE_IDX 0

/* static unsigned long gISPSYS_Reg[EAF_IRQ_TYPE_AMOUNT]; */


#define ISP_EAF_BASE                  (EAF_devs[EAF_DEV_NODE_IDX].regs)
/* #define ISP_EAF_BASE                  (gISPSYS_Reg[EAF_DEV_NODE_IDX]) */

#else
#define ISP_EAF_BASE                        (IMGSYS_BASE + 0x2D000)

#endif


static unsigned int g_u4EnableClockCount;

/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32


typedef enum {
	EAF_FRAME_STATUS_EMPTY,	/* 0 */
	EAF_FRAME_STATUS_ENQUE,	/* 1 */
	EAF_FRAME_STATUS_RUNNING,	/* 2 */
	EAF_FRAME_STATUS_FINISHED,	/* 3 */
	EAF_FRAME_STATUS_TOTAL
} EAF_FRAME_STATUS_ENUM;


typedef enum {
	EAF_REQUEST_STATE_EMPTY,	/* 0 */
	EAF_REQUEST_STATE_PENDING,	/* 1 */
	EAF_REQUEST_STATE_RUNNING,	/* 2 */
	EAF_REQUEST_STATE_FINISHED,	/* 3 */
	EAF_REQUEST_STATE_TOTAL
} EAF_REQUEST_STATE_ENUM;


typedef struct {
	EAF_REQUEST_STATE_ENUM State;
	pid_t processID;	/* caller process ID */
	MUINT32 callerID;	/* caller thread ID */
	MUINT32 enqueReqNum;	/* to judge it belongs to which frame package */
	MINT32 FrameWRIdx;	/* Frame write Index */
	MINT32 FrameRDIdx;	/* Frame read Index */
	EAF_FRAME_STATUS_ENUM EafFrameStatus[_SUPPORT_MAX_EAF_FRAME_REQUEST_];
	EAF_Config EafFrameConfig[_SUPPORT_MAX_EAF_FRAME_REQUEST_];
} EAF_REQUEST_STRUCT;


typedef struct {
	MINT32 WriteIdx;	/* enque how many request  */
	MINT32 ReadIdx;		/* read which request index */
	MINT32 HWProcessIdx;	/* HWWriteIdx */
	EAF_REQUEST_STRUCT EAFReq_Struct[_SUPPORT_MAX_EAF_REQUEST_RING_SIZE_];
} EAF_REQUEST_RING_STRUCT;


typedef struct {
	EAF_Config EafFrameConfig[_SUPPORT_MAX_EAF_FRAME_REQUEST_];
} EAF_CONFIG_STRUCT;
static EAF_REQUEST_RING_STRUCT g_EAF_ReqRing;
static EAF_CONFIG_STRUCT g_EafEnqueReq_Struct;
static EAF_CONFIG_STRUCT g_EafDequeReq_Struct;


/*******************************************************************************
*
********************************************************************************/
typedef struct {
	pid_t Pid;
	pid_t Tid;
} EAF_USER_INFO_STRUCT;

typedef enum {
	EAF_PROCESS_ID_NONE,
	EAF_PROCESS_ID_EAF,
	EAF_PROCESS_ID_AMOUNT
} EAF_PROCESS_ID_ENUM;

/*******************************************************************************
*
********************************************************************************/
typedef struct {
	volatile MUINT32 Status[EAF_IRQ_TYPE_AMOUNT];
	MUINT32 Mask[EAF_IRQ_TYPE_AMOUNT];
	MINT32 EafIrqCnt;
	pid_t ProcessID[EAF_PROCESS_ID_AMOUNT];
} EAF_IRQ_INFO_STRUCT;


typedef struct {
	spinlock_t SpinLockEAFRef;
	spinlock_t SpinLockEAF;
	spinlock_t SpinLockIrq[EAF_IRQ_TYPE_AMOUNT];
	wait_queue_head_t WaitQueueHead;
	struct work_struct ScheduleEafWork;
	MUINT32 UserCount;	/* User Count */
	MUINT32 DebugMask;	/* Debug Mask */
	MINT32 IrqNum;
	EAF_IRQ_INFO_STRUCT IrqInfo;
	signed int WriteReqIdx;
	signed int ReadReqIdx;
	pid_t ProcessID[_SUPPORT_MAX_EAF_FRAME_REQUEST_];
} EAF_INFO_STRUCT;


static EAF_INFO_STRUCT EAFInfo;

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
static SV_LOG_STR gSvLog[EAF_IRQ_TYPE_AMOUNT];

/*
 * for irq used,keep log until IRQ_LOG_PRINTER being involked,
 * limited:
 * each log must shorter than 512 bytes
 * total log length in each irq/logtype can't over 1024 bytes
*/
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



	/* EAF registers */
#define EAF_MAIN_CFG0_HW	(EAF_BASE_HW)
#define EAF_MAIN_CFG1_HW	(EAF_BASE_HW + 0x04)
#define EAF_MAIN_CFG2_HW	(EAF_BASE_HW + 0x08)
#define EAF_MAIN_CFG3_HW	(EAF_BASE_HW + 0x0C)
#define EAF_HIST_CFG0_HW	(EAF_BASE_HW + 0x10)
#define EAF_HIST_CFG1_HW	(EAF_BASE_HW + 0x14)
#define EAF_SRZ_CFG0_HW		(EAF_BASE_HW + 0x18)
#define EAF_BOXF_CFG0_HW	(EAF_BASE_HW + 0x1C)
#define EAF_DIV_CFG0_HW		(EAF_BASE_HW + 0x20)
#define EAF_LKHF_CFG0_HW	(EAF_BASE_HW + 0x24)
#define EAF_LKHF_CFG1_HW	(EAF_BASE_HW + 0x28)
#define EAFI_MASK0_HW		(EAF_BASE_HW + 0x30)
#define EAFI_MASK1_HW		(EAF_BASE_HW + 0x34)
#define EAFI_MASK2_HW		(EAF_BASE_HW + 0x38)
#define EAFI_MASK3_HW		(EAF_BASE_HW + 0x3C)
#define EAFI_CUR_Y0_HW		(EAF_BASE_HW + 0x40)
#define EAFI_CUR_Y1_HW		(EAF_BASE_HW + 0x44)
#define EAFI_CUR_Y2_HW		(EAF_BASE_HW + 0x48)
#define EAFI_CUR_Y3_HW		(EAF_BASE_HW + 0x4C)
#define EAFI_CUR_UV0_HW		(EAF_BASE_HW + 0x50)
#define EAFI_CUR_UV1_HW		(EAF_BASE_HW + 0x54)
#define EAFI_CUR_UV2_HW		(EAF_BASE_HW + 0x58)
#define EAFI_CUR_UV3_HW		(EAF_BASE_HW + 0x5C)
#define EAFI_PRE_Y0_HW		(EAF_BASE_HW + 0x60)
#define EAFI_PRE_Y1_HW		(EAF_BASE_HW + 0x64)
#define EAFI_PRE_Y2_HW		(EAF_BASE_HW + 0x68)
#define EAFI_PRE_Y3_HW		(EAF_BASE_HW + 0x6C)
#define EAFI_PRE_UV0_HW		(EAF_BASE_HW + 0x70)
#define EAFI_PRE_UV1_HW		(EAF_BASE_HW + 0x74)
#define EAFI_PRE_UV2_HW		(EAF_BASE_HW + 0x78)
#define EAFI_PRE_UV3_HW		(EAF_BASE_HW + 0x7C)
#define EAFI_DEP0_HW		(EAF_BASE_HW + 0x80)
#define EAFI_DEP1_HW		(EAF_BASE_HW + 0x84)
#define EAFI_DEP2_HW		(EAF_BASE_HW + 0x88)
#define EAFI_DEP3_HW		(EAF_BASE_HW + 0x8C)
#define EAFI_LKH_WMAP0_HW	(EAF_BASE_HW + 0x90)
#define EAFI_LKH_WMAP1_HW	(EAF_BASE_HW + 0x94)
#define EAFI_LKH_WMAP2_HW	(EAF_BASE_HW + 0x98)
#define EAFI_LKH_WMAP3_HW	(EAF_BASE_HW + 0x9C)
#define EAFI_LKH_EMAP0_HW	(EAF_BASE_HW + 0xA0)
#define EAFI_LKH_EMAP1_HW	(EAF_BASE_HW + 0xA4)
#define EAFI_LKH_EMAP2_HW	(EAF_BASE_HW + 0xA8)
#define EAFI_LKH_EMAP3_HW	(EAF_BASE_HW + 0xAC)
#define EAFI_PROB0_HW		(EAF_BASE_HW + 0xB0)
#define EAFI_PROB1_HW		(EAF_BASE_HW + 0xB4)
#define EAFI_PROB2_HW		(EAF_BASE_HW + 0xB8)
#define EAFI_PROB3_HW		(EAF_BASE_HW + 0xBC)
#define EAFO_FOUT0_HW		(EAF_BASE_HW + 0xC0)
#define EAFO_FOUT1_HW		(EAF_BASE_HW + 0xC4)
#define EAFO_FOUT2_HW		(EAF_BASE_HW + 0xC8)
#define EAFO_FOUT3_HW		(EAF_BASE_HW + 0xCC)
#define EAFO_FOUT4_HW		(EAF_BASE_HW + 0xD0)
#define EAFO_POUT0_HW		(EAF_BASE_HW + 0xE0)
#define EAFO_POUT1_HW		(EAF_BASE_HW + 0xE4)
#define EAFO_POUT2_HW		(EAF_BASE_HW + 0xE8)
#define EAFO_POUT3_HW		(EAF_BASE_HW + 0xEC)
#define EAFO_POUT4_HW		(EAF_BASE_HW + 0xF0)
#define EAF_MASK_LB_CTL0_HW	(EAF_BASE_HW + 0x100)
#define EAF_MASK_LB_CTL1_HW	(EAF_BASE_HW + 0x104)
#define EAF_PRE_Y_LB_CTL0_HW	(EAF_BASE_HW + 0x110)
#define EAF_PRE_Y_LB_CTL1_HW	(EAF_BASE_HW + 0x114)
#define EAF_PRE_UV_LB_CTL0_HW	(EAF_BASE_HW + 0x120)
#define EAF_PRE_UV_LB_CTL1_HW	(EAF_BASE_HW + 0x124)
#define EAF_CUR_UV_LB_CTL0_HW	(EAF_BASE_HW + 0x130)
#define EAF_CUR_UV_LB_CTL1_HW	(EAF_BASE_HW + 0x134)
#define EAF_DMA_DCM_DIS_HW		(EAF_BASE_HW + 0x200)
#define EAF_MAIN_DCM_DIS_HW		(EAF_BASE_HW + 0x204)
#define EAF_DMA_DCM_ST_HW		(EAF_BASE_HW + 0x208)
#define EAF_MAIN_DCM_ST_HW		(EAF_BASE_HW + 0x20C)
#define EAF_INT_CTL_HW			(EAF_BASE_HW + 0x210)
#define EAF_INT_STATUS_HW		(EAF_BASE_HW + 0x214)
#define EAF_SW_RST_HW			(EAF_BASE_HW + 0x220)
#define EAF_DBG_CTL_HW			(EAF_BASE_HW + 0x230)
#define EAF_DBG_MON0_HW		(EAF_BASE_HW + 0x234)
#define EAF_DBG_MON1_HW		(EAF_BASE_HW + 0x238)
#define EAF_DBG_MON2_HW		(EAF_BASE_HW + 0x23C)
#define EAF_DBG_MON3_HW		(EAF_BASE_HW + 0x240)
#define EAF_DBG_MON4_HW		(EAF_BASE_HW + 0x244)
#define EAF_DBG_MON5_HW		(EAF_BASE_HW + 0x248)
#define EAF_DBG_MON6_HW		(EAF_BASE_HW + 0x24C)
#define EAF_DBG_MON7_HW		(EAF_BASE_HW + 0x250)
#define EAF_DBG_MON8_HW		(EAF_BASE_HW + 0x254)
#define EAF_DBG_MON9_HW		(EAF_BASE_HW + 0x258)


#define  JBFR_CFG0_HW	(EAF_BASE_HW + 0x300 + 0x0)
#define  JBFR_CFG1_HW	(EAF_BASE_HW + 0x300 + 0x4)
#define  JBFR_CFG2_HW	(EAF_BASE_HW + 0x300 + 0x8)
#define  JBFR_CFG3_HW	(EAF_BASE_HW + 0x300 + 0xC)
#define  JBFR_CFG4_HW	(EAF_BASE_HW + 0x300 + 0x10)
#define  JBFR_CFG5_HW	(EAF_BASE_HW + 0x300 + 0x14)
#define  JBFR_SIZE_HW	(EAF_BASE_HW + 0x300 + 0x18)
#define  JBFR_CFG6_HW	(EAF_BASE_HW + 0x300 + 0x20)
#define  JBFR_CFG7_HW	(EAF_BASE_HW + 0x300 + 0x24)
#define  JBFR_CFG8_HW	(EAF_BASE_HW + 0x300 + 0x28)


#define  SRZ6_CONTROL_HW	(EAF_BASE_HW + 0x380 + 0x0)
#define  SRZ6_IN_IMG_HW		(EAF_BASE_HW + 0x380 + 0x4)
#define  SRZ6_OUT_IMG_HW	(EAF_BASE_HW + 0x380 + 0x8)
#define  SRZ6_HORI_STEP_HW	(EAF_BASE_HW + 0x380 + 0xc)
#define  SRZ6_VERT_STEP_HW	(EAF_BASE_HW + 0x380 + 0x10)
#define  SRZ6_HORI_INT_OFST_HW	(EAF_BASE_HW + 0x380 + 0x14)
#define  SRZ6_HORI_SUB_OFST_HW	(EAF_BASE_HW + 0x380 + 0x18)
#define  SRZ6_VERT_INT_OFST_HW	(EAF_BASE_HW + 0x380 + 0x1c)
#define  SRZ6_VERT_SUB_OFST_HW	(EAF_BASE_HW + 0x380 + 0x20)


#define  EAF_DMA_SOFT_RSTSTAT_HW	(EAF_BASE_HW + 0x800 + 0x0)
#define  EAF_TDRI_BASE_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x4)
#define  EAF_TDRI_OFST_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x8)
#define  EAF_TDRI_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0xc)
#define  EAF_VERTICAL_FLIP_EN_HW	(EAF_BASE_HW + 0x800 + 0x10)
#define  EAF_DMA_SOFT_RESET_HW	    (EAF_BASE_HW + 0x800 + 0x14)
#define  EAF_LAST_ULTRA_EN_HW	    (EAF_BASE_HW + 0x800 + 0x18)
#define  EAF_SPECIAL_FUN_EN_HW	    (EAF_BASE_HW + 0x800 + 0x1c)
#define  EAFO_BASE_ADDR_HW	        (EAF_BASE_HW + 0x800 + 0x30)
#define  EAFO_OFST_ADDR_HW	        (EAF_BASE_HW + 0x800 + 0x38)
#define  EAFO_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x40)
#define  EAFO_YSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x44)
#define  EAFO_STRIDE_HW	        (EAF_BASE_HW + 0x800 + 0x48)
#define  EAFO_CON_HW	        (EAF_BASE_HW + 0x800 + 0x4c)
#define  EAFO_CON2_HW	        (EAF_BASE_HW + 0x800 + 0x50)
#define  EAFO_CON3_HW	        (EAF_BASE_HW + 0x800 + 0x54)
#define  EAFO_CROP_HW	        (EAF_BASE_HW + 0x800 + 0x58)
#define  EAFI_BASE_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x90)
#define  EAFI_OFST_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x98)
#define  EAFI_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0xa0)
#define  EAFI_YSIZE_HW	        (EAF_BASE_HW + 0x800 + 0xa4)
#define  EAFI_STRIDE_HW	        (EAF_BASE_HW + 0x800 + 0xa8)
#define  EAFI_CON_HW	        (EAF_BASE_HW + 0x800 + 0xac)
#define  EAFI_CON2_HW	        (EAF_BASE_HW + 0x800 + 0xb0)
#define  EAFI_CON3_HW	        (EAF_BASE_HW + 0x800 + 0xb4)
#define  EAF2I_BASE_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0xc0)
#define  EAF2I_OFST_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0xc8)
#define  EAF2I_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0xd0)
#define  EAF2I_YSIZE_HW	        (EAF_BASE_HW + 0x800 + 0xd4)
#define  EAF2I_STRIDE_HW	    (EAF_BASE_HW + 0x800 + 0xd8)
#define  EAF2I_CON_HW	        (EAF_BASE_HW + 0x800 + 0xdc)
#define  EAF2I_CON2_HW	        (EAF_BASE_HW + 0x800 + 0xe0)
#define  EAF2I_CON3_HW	        (EAF_BASE_HW + 0x800 + 0xe4)
#define  EAF3I_BASE_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0xf0)
#define  EAF3I_OFST_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0xf8)
#define  EAF3I_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x100)
#define  EAF3I_YSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x104)
#define  EAF3I_STRIDE_HW	    (EAF_BASE_HW + 0x800 + 0x108)
#define  EAF3I_CON_HW	        (EAF_BASE_HW + 0x800 + 0x10c)
#define  EAF3I_CON2_HW	        (EAF_BASE_HW + 0x800 + 0x110)
#define  EAF3I_CON3_HW	        (EAF_BASE_HW + 0x800 + 0x114)
#define  EAF4I_BASE_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x120)
#define  EAF4I_OFST_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x128)
#define  EAF4I_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x130)
#define  EAF4I_YSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x134)
#define  EAF4I_STRIDE_HW	    (EAF_BASE_HW + 0x800 + 0x138)
#define  EAF4I_CON_HW	        (EAF_BASE_HW + 0x800 + 0x13c)
#define  EAF4I_CON2_HW	        (EAF_BASE_HW + 0x800 + 0x140)
#define  EAF4I_CON3_HW	        (EAF_BASE_HW + 0x800 + 0x144)
#define  EAF5I_BASE_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x150)
#define  EAF5I_OFST_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x158)
#define  EAF5I_XSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x160)
#define  EAF5I_YSIZE_HW	        (EAF_BASE_HW + 0x800 + 0x164)
#define  EAF5I_STRIDE_HW	    (EAF_BASE_HW + 0x800 + 0x168)
#define  EAF5I_CON_HW	        (EAF_BASE_HW + 0x800 + 0x16c)
#define  EAF5I_CON2_HW	        (EAF_BASE_HW + 0x800 + 0x170)
#define  EAF5I_CON3_HW	        (EAF_BASE_HW + 0x800 + 0x174)
#define  EAF6I_BASE_ADDR_HW		(EAF_BASE_HW + 0x800 + 0x180)
#define  EAF6I_OFST_ADDR_HW		(EAF_BASE_HW + 0x800 + 0x188)
#define  EAF6I_XSIZE_HW		(EAF_BASE_HW + 0x800 + 0x190)
#define  EAF6I_YSIZE_HW		(EAF_BASE_HW + 0x800 + 0x194)
#define  EAF6I_STRIDE_HW	    (EAF_BASE_HW + 0x800 + 0x198)
#define  EAF6I_CON_HW	        (EAF_BASE_HW + 0x800 + 0x19c)
#define  EAF6I_CON2_HW	        (EAF_BASE_HW + 0x800 + 0x1a0)
#define  EAF6I_CON3_HW	        (EAF_BASE_HW + 0x800 + 0x1a4)
#define  EAF_DMA_ERR_CTRL_HW	    (EAF_BASE_HW + 0x800 + 0x200)
#define  EAFO_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x204)
#define  EAFI_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x208)
#define  EAF2I_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x20c)
#define  EAF3I_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x210)
#define  EAF4I_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x214)
#define  EAF5I_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x218)
#define  EAF6I_ERR_STAT_HW	        (EAF_BASE_HW + 0x800 + 0x21c)
#define  EAF_DMA_DEBUG_ADDR_HW	    (EAF_BASE_HW + 0x800 + 0x220)
#define  EAF_DMA_RSV1_HW	        (EAF_BASE_HW + 0x800 + 0x224)
#define  EAF_DMA_RSV2_HW	        (EAF_BASE_HW + 0x800 + 0x228)
#define  EAF_DMA_RSV3_HW	        (EAF_BASE_HW + 0x800 + 0x22c)
#define  EAF_DMA_RSV4_HW	        (EAF_BASE_HW + 0x800 + 0x230)
#define  EAF_DMA_DEBUG_SEL_HW	    (EAF_BASE_HW + 0x800 + 0x234)
#define  EAF_DMA_BW_SELF_TEST_HW	(EAF_BASE_HW + 0x800 + 0x238)

/*SW Access Registers : using mapped base address from DTS*/
#define EAF_MAIN_CFG0_REG	(ISP_EAF_BASE)
#define EAF_MAIN_CFG1_REG	(ISP_EAF_BASE + 0x04)
#define EAF_MAIN_CFG2_REG	(ISP_EAF_BASE + 0x08)
#define EAF_MAIN_CFG3_REG	(ISP_EAF_BASE + 0x0C)
#define EAF_HIST_CFG0_REG	(ISP_EAF_BASE + 0x10)
#define EAF_HIST_CFG1_REG	(ISP_EAF_BASE + 0x14)
#define EAF_SRZ_CFG0_REG		(ISP_EAF_BASE + 0x18)
#define EAF_BOXF_CFG0_REG	(ISP_EAF_BASE + 0x1C)
#define EAF_DIV_CFG0_REG		(ISP_EAF_BASE + 0x20)
#define EAF_LKHF_CFG0_REG	(ISP_EAF_BASE + 0x24)
#define EAF_LKHF_CFG1_REG	(ISP_EAF_BASE + 0x28)
#define EAFI_MASK0_REG		(ISP_EAF_BASE + 0x30)
#define EAFI_MASK1_REG		(ISP_EAF_BASE + 0x34)
#define EAFI_MASK2_REG		(ISP_EAF_BASE + 0x38)
#define EAFI_MASK3_REG		(ISP_EAF_BASE + 0x3C)
#define EAFI_CUR_Y0_REG		(ISP_EAF_BASE + 0x40)
#define EAFI_CUR_Y1_REG		(ISP_EAF_BASE + 0x44)
#define EAFI_CUR_Y2_REG		(ISP_EAF_BASE + 0x48)
#define EAFI_CUR_Y3_REG		(ISP_EAF_BASE + 0x4C)
#define EAFI_CUR_UV0_REG		(ISP_EAF_BASE + 0x50)
#define EAFI_CUR_UV1_REG		(ISP_EAF_BASE + 0x54)
#define EAFI_CUR_UV2_REG		(ISP_EAF_BASE + 0x58)
#define EAFI_CUR_UV3_REG		(ISP_EAF_BASE + 0x5C)
#define EAFI_PRE_Y0_REG		(ISP_EAF_BASE + 0x60)
#define EAFI_PRE_Y1_REG		(ISP_EAF_BASE + 0x64)
#define EAFI_PRE_Y2_REG		(ISP_EAF_BASE + 0x68)
#define EAFI_PRE_Y3_REG		(ISP_EAF_BASE + 0x6C)
#define EAFI_PRE_UV0_REG		(ISP_EAF_BASE + 0x70)
#define EAFI_PRE_UV1_REG		(ISP_EAF_BASE + 0x74)
#define EAFI_PRE_UV2_REG		(ISP_EAF_BASE + 0x78)
#define EAFI_PRE_UV3_REG		(ISP_EAF_BASE + 0x7C)
#define EAFI_DEP0_REG		(ISP_EAF_BASE + 0x80)
#define EAFI_DEP1_REG		(ISP_EAF_BASE + 0x84)
#define EAFI_DEP2_REG		(ISP_EAF_BASE + 0x88)
#define EAFI_DEP3_REG		(ISP_EAF_BASE + 0x8C)
#define EAFI_LKH_WMAP0_REG	(ISP_EAF_BASE + 0x90)
#define EAFI_LKH_WMAP1_REG	(ISP_EAF_BASE + 0x94)
#define EAFI_LKH_WMAP2_REG	(ISP_EAF_BASE + 0x98)
#define EAFI_LKH_WMAP3_REG	(ISP_EAF_BASE + 0x9C)
#define EAFI_LKH_EMAP0_REG	(ISP_EAF_BASE + 0xA0)
#define EAFI_LKH_EMAP1_REG	(ISP_EAF_BASE + 0xA4)
#define EAFI_LKH_EMAP2_REG	(ISP_EAF_BASE + 0xA8)
#define EAFI_LKH_EMAP3_REG	(ISP_EAF_BASE + 0xAC)
#define EAFI_PROB0_REG		(ISP_EAF_BASE + 0xB0)
#define EAFI_PROB1_REG		(ISP_EAF_BASE + 0xB4)
#define EAFI_PROB2_REG		(ISP_EAF_BASE + 0xB8)
#define EAFI_PROB3_REG		(ISP_EAF_BASE + 0xBC)
#define EAFO_FOUT0_REG		(ISP_EAF_BASE + 0xC0)
#define EAFO_FOUT1_REG		(ISP_EAF_BASE + 0xC4)
#define EAFO_FOUT2_REG		(ISP_EAF_BASE + 0xC8)
#define EAFO_FOUT3_REG		(ISP_EAF_BASE + 0xCC)
#define EAFO_FOUT4_REG		(ISP_EAF_BASE + 0xD0)
#define EAFO_POUT0_REG		(ISP_EAF_BASE + 0xE0)
#define EAFO_POUT1_REG		(ISP_EAF_BASE + 0xE4)
#define EAFO_POUT2_REG		(ISP_EAF_BASE + 0xE8)
#define EAFO_POUT3_REG		(ISP_EAF_BASE + 0xEC)
#define EAFO_POUT4_REG		(ISP_EAF_BASE + 0xF0)
#define EAF_MASK_LB_CTL0_REG	(ISP_EAF_BASE + 0x100)
#define EAF_MASK_LB_CTL1_REG	(ISP_EAF_BASE + 0x104)
#define EAF_PRE_Y_LB_CTL0_REG	(ISP_EAF_BASE + 0x110)
#define EAF_PRE_Y_LB_CTL1_REG	(ISP_EAF_BASE + 0x114)
#define EAF_PRE_UV_LB_CTL0_REG	(ISP_EAF_BASE + 0x120)
#define EAF_PRE_UV_LB_CTL1_REG	(ISP_EAF_BASE + 0x124)
#define EAF_CUR_UV_LB_CTL0_REG	(ISP_EAF_BASE + 0x130)
#define EAF_CUR_UV_LB_CTL1_REG	(ISP_EAF_BASE + 0x134)
#define EAF_DMA_DCM_DIS_REG		(ISP_EAF_BASE + 0x200)
#define EAF_MAIN_DCM_DIS_REG		(ISP_EAF_BASE + 0x204)
#define EAF_DMA_DCM_ST_REG		(ISP_EAF_BASE + 0x208)
#define EAF_MAIN_DCM_ST_REG		(ISP_EAF_BASE + 0x20C)
#define EAF_INT_CTL_REG			(ISP_EAF_BASE + 0x210)
#define EAF_INT_STATUS_REG		(ISP_EAF_BASE + 0x214)
#define EAF_SW_RST_REG			(ISP_EAF_BASE + 0x220)
#define EAF_DBG_CTL_REG			(ISP_EAF_BASE + 0x230)
#define EAF_DBG_MON0_REG		(ISP_EAF_BASE + 0x234)
#define EAF_DBG_MON1_REG		(ISP_EAF_BASE + 0x238)
#define EAF_DBG_MON2_REG		(ISP_EAF_BASE + 0x23C)
#define EAF_DBG_MON3_REG		(ISP_EAF_BASE + 0x240)
#define EAF_DBG_MON4_REG		(ISP_EAF_BASE + 0x244)
#define EAF_DBG_MON5_REG		(ISP_EAF_BASE + 0x248)
#define EAF_DBG_MON6_REG		(ISP_EAF_BASE + 0x24C)
#define EAF_DBG_MON7_REG		(ISP_EAF_BASE + 0x250)
#define EAF_DBG_MON8_REG		(ISP_EAF_BASE + 0x254)
#define EAF_DBG_MON9_REG		(ISP_EAF_BASE + 0x258)


#define  JBFR_CFG0_REG	(ISP_EAF_BASE + 0x300 + 0x0)
#define  JBFR_CFG1_REG	(ISP_EAF_BASE + 0x300 + 0x4)
#define  JBFR_CFG2_REG	(ISP_EAF_BASE + 0x300 + 0x8)
#define  JBFR_CFG3_REG	(ISP_EAF_BASE + 0x300 + 0xC)
#define  JBFR_CFG4_REG	(ISP_EAF_BASE + 0x300 + 0x10)
#define  JBFR_CFG5_REG	(ISP_EAF_BASE + 0x300 + 0x14)
#define  JBFR_SIZE_REG	(ISP_EAF_BASE + 0x300 + 0x18)
#define  JBFR_CFG6_REG	(ISP_EAF_BASE + 0x300 + 0x20)
#define  JBFR_CFG7_REG	(ISP_EAF_BASE + 0x300 + 0x24)
#define  JBFR_CFG8_REG	(ISP_EAF_BASE + 0x300 + 0x28)


#define  SRZ6_CONTROL_REG	(ISP_EAF_BASE + 0x380 + 0x0)
#define  SRZ6_IN_IMG_REG		(ISP_EAF_BASE + 0x380 + 0x4)
#define  SRZ6_OUT_IMG_REG	(ISP_EAF_BASE + 0x380 + 0x8)
#define  SRZ6_HORI_STEP_REG	(ISP_EAF_BASE + 0x380 + 0xc)
#define  SRZ6_VERT_STEP_REG	(ISP_EAF_BASE + 0x380 + 0x10)
#define  SRZ6_HORI_INT_OFST_REG	(ISP_EAF_BASE + 0x380 + 0x14)
#define  SRZ6_HORI_SUB_OFST_REG	(ISP_EAF_BASE + 0x380 + 0x18)
#define  SRZ6_VERT_INT_OFST_REG	(ISP_EAF_BASE + 0x380 + 0x1c)
#define  SRZ6_VERT_SUB_OFST_REG	(ISP_EAF_BASE + 0x380 + 0x20)


#define  EAF_DMA_SOFT_RSTSTAT_REG	(ISP_EAF_BASE + 0x800 + 0x0)
#define  EAF_TDRI_BASE_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x4)
#define  EAF_TDRI_OFST_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x8)
#define  EAF_TDRI_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0xc)
#define  EAF_VERTICAL_FLIP_EN_REG	(ISP_EAF_BASE + 0x800 + 0x10)
#define  EAF_DMA_SOFT_RESET_REG	    (ISP_EAF_BASE + 0x800 + 0x14)
#define  EAF_LAST_ULTRA_EN_REG	    (ISP_EAF_BASE + 0x800 + 0x18)
#define  EAF_SPECIAL_FUN_EN_REG	    (ISP_EAF_BASE + 0x800 + 0x1c)
#define  EAFO_BASE_ADDR_REG	        (ISP_EAF_BASE + 0x800 + 0x30)
#define  EAFO_OFST_ADDR_REG	        (ISP_EAF_BASE + 0x800 + 0x38)
#define  EAFO_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x40)
#define  EAFO_YSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x44)
#define  EAFO_STRIDE_REG	        (ISP_EAF_BASE + 0x800 + 0x48)
#define  EAFO_CON_REG	        (ISP_EAF_BASE + 0x800 + 0x4c)
#define  EAFO_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0x50)
#define  EAFO_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0x54)
#define  EAFO_CROP_REG	        (ISP_EAF_BASE + 0x800 + 0x58)
#define  EAFI_BASE_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x90)
#define  EAFI_OFST_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x98)
#define  EAFI_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0xa0)
#define  EAFI_YSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0xa4)
#define  EAFI_STRIDE_REG	        (ISP_EAF_BASE + 0x800 + 0xa8)
#define  EAFI_CON_REG	        (ISP_EAF_BASE + 0x800 + 0xac)
#define  EAFI_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0xb0)
#define  EAFI_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0xb4)
#define  EAF2I_BASE_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0xc0)
#define  EAF2I_OFST_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0xc8)
#define  EAF2I_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0xd0)
#define  EAF2I_YSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0xd4)
#define  EAF2I_STRIDE_REG	    (ISP_EAF_BASE + 0x800 + 0xd8)
#define  EAF2I_CON_REG	        (ISP_EAF_BASE + 0x800 + 0xdc)
#define  EAF2I_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0xe0)
#define  EAF2I_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0xe4)
#define  EAF3I_BASE_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0xf0)
#define  EAF3I_OFST_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0xf8)
#define  EAF3I_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x100)
#define  EAF3I_YSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x104)
#define  EAF3I_STRIDE_REG	    (ISP_EAF_BASE + 0x800 + 0x108)
#define  EAF3I_CON_REG	        (ISP_EAF_BASE + 0x800 + 0x10c)
#define  EAF3I_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0x110)
#define  EAF3I_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0x114)
#define  EAF4I_BASE_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x120)
#define  EAF4I_OFST_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x128)
#define  EAF4I_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x130)
#define  EAF4I_YSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x134)
#define  EAF4I_STRIDE_REG	    (ISP_EAF_BASE + 0x800 + 0x138)
#define  EAF4I_CON_REG	        (ISP_EAF_BASE + 0x800 + 0x13c)
#define  EAF4I_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0x140)
#define  EAF4I_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0x144)
#define  EAF5I_BASE_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x150)
#define  EAF5I_OFST_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x158)
#define  EAF5I_XSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x160)
#define  EAF5I_YSIZE_REG	        (ISP_EAF_BASE + 0x800 + 0x164)
#define  EAF5I_STRIDE_REG	    (ISP_EAF_BASE + 0x800 + 0x168)
#define  EAF5I_CON_REG	        (ISP_EAF_BASE + 0x800 + 0x16c)
#define  EAF5I_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0x170)
#define  EAF5I_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0x174)
#define  EAF6I_BASE_ADDR_REG		(ISP_EAF_BASE + 0x800 + 0x180)
#define  EAF6I_OFST_ADDR_REG		(ISP_EAF_BASE + 0x800 + 0x188)
#define  EAF6I_XSIZE_REG		(ISP_EAF_BASE + 0x800 + 0x190)
#define  EAF6I_YSIZE_REG		(ISP_EAF_BASE + 0x800 + 0x194)
#define  EAF6I_STRIDE_REG	    (ISP_EAF_BASE + 0x800 + 0x198)
#define  EAF6I_CON_REG	        (ISP_EAF_BASE + 0x800 + 0x19c)
#define  EAF6I_CON2_REG	        (ISP_EAF_BASE + 0x800 + 0x1a0)
#define  EAF6I_CON3_REG	        (ISP_EAF_BASE + 0x800 + 0x1a4)
#define  EAF_DMA_ERR_CTRL_REG	    (ISP_EAF_BASE + 0x800 + 0x200)
#define  EAFO_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x204)
#define  EAFI_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x208)
#define  EAF2I_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x20c)
#define  EAF3I_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x210)
#define  EAF4I_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x214)
#define  EAF5I_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x218)
#define  EAF6I_ERR_STAT_REG	        (ISP_EAF_BASE + 0x800 + 0x21c)
#define  EAF_DMA_DEBUG_ADDR_REG	    (ISP_EAF_BASE + 0x800 + 0x220)
#define  EAF_DMA_RSV1_REG	        (ISP_EAF_BASE + 0x800 + 0x224)
#define  EAF_DMA_RSV2_REG	        (ISP_EAF_BASE + 0x800 + 0x228)
#define  EAF_DMA_RSV3_REG	        (ISP_EAF_BASE + 0x800 + 0x22c)
#define  EAF_DMA_RSV4_REG	        (ISP_EAF_BASE + 0x800 + 0x230)
#define  EAF_DMA_DEBUG_SEL_REG	    (ISP_EAF_BASE + 0x800 + 0x234)
#define  EAF_DMA_BW_SELF_TEST_REG	(ISP_EAF_BASE + 0x800 + 0x238)

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 EAF_MsToJiffies(MUINT32 Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 EAF_UsToJiffies(MUINT32 Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 EAF_GetIRQState(MUINT32 type, MUINT32 userNumber, MUINT32 stus,
					  EAF_PROCESS_ID_ENUM whichReq, int ProcessID)
{
	MUINT32 ret = 0;
	unsigned long flags;	/* old: MUINT32 flags; *//* FIX to avoid build warning */

	/*	*/
	spin_lock_irqsave(&(EAFInfo.SpinLockIrq[type]), flags);

#ifdef EAF_MULTIPROCESS_TIMEING_ISSUE

	if (stus & EAF_INT_ST) {
		ret = ((EAFInfo.IrqInfo.EafIrqCnt > 0)
			   && (EAFInfo.ProcessID[EAFInfo.ReadReqIdx] == ProcessID));
	} else {
		LOG_ERR
			(" WIRQ StaErr, type:%d, usNum:%d, status:%d, whichReq:%d,PID:0x%x, ReadReqIdx:%d\n",
			 type, userNumber, stus, whichReq, ProcessID, EAFInfo.ReadReqIdx);
	}

#else
	if (stus & EAF_INT_ST) {
		ret = ((EAFInfo.IrqInfo.EafIrqCnt > 0)
			   && (EAFInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
	} else {
		LOG_ERR
			("WaitIRQ Status Error, type:%d, userNumber:%d, status:%d, whichReq:%d, ProcessID:0x%x\n",
			 type, userNumber, stus, whichReq, ProcessID);
	}
#endif

	spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[type]), flags);
	/*	*/
	return ret;
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 EAF_JiffiesToMs(MUINT32 Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}


#define RegDump(start, end) {\
	MUINT32 i;\
	for (i = start; i <= end; i += 0x10) {\
		LOG_DBG("[0x%08X %08X],[0x%08X %08X],[0x%08X %08X],[0x%08X %08X]",\
	    (unsigned int)(ISP_EAF_BASE + i), (unsigned int)EAF_RD32(ISP_EAF_BASE + i),\
	    (unsigned int)(ISP_EAF_BASE + i+0x4), (unsigned int)EAF_RD32(ISP_EAF_BASE + i+0x4),\
	    (unsigned int)(ISP_EAF_BASE + i+0x8), (unsigned int)EAF_RD32(ISP_EAF_BASE + i+0x8),\
	    (unsigned int)(ISP_EAF_BASE + i+0xc), (unsigned int)EAF_RD32(ISP_EAF_BASE + i+0xc));\
	} \
}


static MBOOL ConfigEAFRequest(MINT32 ReqIdx)
{
	MUINT32 j;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */


	spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
	if (g_EAF_ReqRing.EAFReq_Struct[ReqIdx].State == EAF_REQUEST_STATE_PENDING) {
		g_EAF_ReqRing.EAFReq_Struct[ReqIdx].State = EAF_REQUEST_STATE_RUNNING;
		for (j = 0; j < _SUPPORT_MAX_EAF_FRAME_REQUEST_; j++) {
			if (EAF_FRAME_STATUS_ENQUE ==
			    g_EAF_ReqRing.EAFReq_Struct[ReqIdx].EafFrameStatus[j]) {
				g_EAF_ReqRing.EAFReq_Struct[ReqIdx].EafFrameStatus[j] = EAF_FRAME_STATUS_RUNNING;
				spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
				ConfigEAFHW(&g_EAF_ReqRing.EAFReq_Struct[ReqIdx].EafFrameConfig[j]);
				spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
			}
		}
	} else {
		LOG_ERR("ConfigEAFRequest state machine error!!, ReqIdx:%d, State:%d\n",
			ReqIdx, g_EAF_ReqRing.EAFReq_Struct[ReqIdx].State);
	}
	spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);

	return MTRUE;
}


static MBOOL ConfigEAF(void)
{
	MUINT32 i, j, k;
	unsigned long flags; /* old: MUINT32 flags;*//* FIX to avoid build warning */

	LOG_INF("ConfigEAF\n");

	spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
	for (k = 0; k < _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_; k++) {
		i = (g_EAF_ReqRing.HWProcessIdx + k) % _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
		if (g_EAF_ReqRing.EAFReq_Struct[i].State == EAF_REQUEST_STATE_PENDING) {
			g_EAF_ReqRing.EAFReq_Struct[i].State =
			    EAF_REQUEST_STATE_RUNNING;
			for (j = 0; j < _SUPPORT_MAX_EAF_FRAME_REQUEST_; j++) {
				if (EAF_FRAME_STATUS_ENQUE ==
				    g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j]) {
					/* break; */
					g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j] = EAF_FRAME_STATUS_RUNNING;
					spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
					ConfigEAFHW(&g_EAF_ReqRing.EAFReq_Struct[i].EafFrameConfig[j]);
					spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
				}
			}
			/* LOG_DBG("ConfigEAF idx j:%d\n",j); */
			if (j != _SUPPORT_MAX_EAF_FRAME_REQUEST_) {
				LOG_ERR
				    ("EAF Config State is wrong!  idx j(%d), HWProcessIdx(%d), State(%d)\n",
				     j, g_EAF_ReqRing.HWProcessIdx,
				     g_EAF_ReqRing.EAFReq_Struct[i].State);
				/* g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j]
				* = EAF_FRAME_STATUS_RUNNING;
				*/
				/* spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags); */
				/* ConfigEAFHW(&g_EAF_ReqRing.EAFReq_Struct[i].EafFrameConfig[j]); */
				/* return MTRUE; */
				return MFALSE;
			}
			/*else {
			*	g_EAF_ReqRing.EAFReq_Struct[i].State = EAF_REQUEST_STATE_RUNNING;
			*	LOG_ERR("EAF Config State is wrong! HWProcessIdx(%d), State(%d)\n",
			*	g_EAF_ReqRing.HWProcessIdx, g_EAF_ReqRing.EAFReq_Struct[i].State);
			*	g_EAF_ReqRing.HWProcessIdx = (g_EAF_ReqRing.HWProcessIdx+1)
			*	%_SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
			*}
			*/
		}
	}
	spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
	if (k == _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_)
		LOG_DBG("No any EAF Request in Ring!!\n");

	return MTRUE;
}


static MBOOL UpdateEAF(pid_t *ProcessID)
{
	MUINT32 i, j, next_idx;
	MBOOL bFinishRequest = MFALSE;

	for (i = g_EAF_ReqRing.HWProcessIdx; i < _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_; i++) {
		if (g_EAF_ReqRing.EAFReq_Struct[i].State == EAF_REQUEST_STATE_RUNNING) {
			for (j = 0; j < _SUPPORT_MAX_EAF_FRAME_REQUEST_; j++) {
				if (EAF_FRAME_STATUS_RUNNING ==
				    g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_DBG,
				       "UpdateEAF idx j:%d\n", j);
			if (j != _SUPPORT_MAX_EAF_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j] =
				    EAF_FRAME_STATUS_FINISHED;
				if ((_SUPPORT_MAX_EAF_FRAME_REQUEST_ == (next_idx))
				    || ((_SUPPORT_MAX_EAF_FRAME_REQUEST_ > (next_idx))
					&& (EAF_FRAME_STATUS_EMPTY ==
					    g_EAF_ReqRing.EAFReq_Struct[i].
					    EafFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) =
					    g_EAF_ReqRing.EAFReq_Struct[i].processID;
					g_EAF_ReqRing.EAFReq_Struct[i].State =
					    EAF_REQUEST_STATE_FINISHED;
					g_EAF_ReqRing.HWProcessIdx =
					    (g_EAF_ReqRing.HWProcessIdx +
					     1) % _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
					IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB,
						       _LOG_INF,
						       "Finish EAF Request i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_EAF_ReqRing.HWProcessIdx);
				} else {
					IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB,
						       _LOG_DBG,
						       "Finish EAF Frame i:%d, j:%d, HWProcessIdx:%d\n",
						       i, j, g_EAF_ReqRing.HWProcessIdx);
				}
				break;
			}
			/*else {*/
			IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_ERR,
				       "EAF State Machine is wrong! HWProcessIdx(%d), State(%d)\n",
				       g_EAF_ReqRing.HWProcessIdx,
				       g_EAF_ReqRing.EAFReq_Struct[i].State);
			g_EAF_ReqRing.EAFReq_Struct[i].State =
			    EAF_REQUEST_STATE_FINISHED;
			g_EAF_ReqRing.HWProcessIdx =
			    (g_EAF_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
			break;
			/*}*/
		}
	}

	return bFinishRequest;
}

static MINT32 ConfigEAFHW(EAF_Config *pEafConfig)
{
#ifdef EAF_USE_GCE
	struct cmdqRecStruct *handle;	/* kernel-4.4 usage */
		/* cmdqRecHandle handle; */ /* kernel-3.18 usage */
	uint64_t engineFlag = (1L << CMDQ_ENG_EAF);
#endif

	LOG_INF("ConfigEAFHW\n");


	if (EAF_DBG_DBGLOG == (EAF_DBG_DBGLOG & EAFInfo.DebugMask)) {

		LOG_ERR("ConfigEAFHW Start!\n");
		LOG_ERR("ConfigEAFHW Start!\n");

	}
#ifdef EAF_USE_GCE

	cmdqRecCreate(CMDQ_SCENARIO_KERNEL_CONFIG_GENERAL, &handle);
	/* CMDQ driver dispatches CMDQ HW thread and HW thread's priority according to scenario */

	cmdqRecSetEngine(handle, engineFlag);

	cmdqRecReset(handle);

	/* Use command queue to write register */
	cmdqRecWrite(handle, EAF_MAIN_CFG0_HW, pEafConfig->EAF_MAIN_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_MAIN_CFG1_HW, pEafConfig->EAF_MAIN_CFG1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_MAIN_CFG2_HW, pEafConfig->EAF_MAIN_CFG2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_MAIN_CFG3_HW, pEafConfig->EAF_MAIN_CFG3, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_HIST_CFG0_HW, pEafConfig->EAF_HIST_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_HIST_CFG1_HW, pEafConfig->EAF_HIST_CFG1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_SRZ_CFG0_HW, pEafConfig->EAF_SRZ_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_BOXF_CFG0_HW, pEafConfig->EAF_BOXF_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_DIV_CFG0_HW, pEafConfig->EAF_DIV_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_LKHF_CFG0_HW, pEafConfig->EAF_LKHF_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_LKHF_CFG1_HW, pEafConfig->EAF_LKHF_CFG1, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_MASK0_HW, pEafConfig->EAFI_MASK0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_MASK1_HW, pEafConfig->EAFI_MASK1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_MASK2_HW, pEafConfig->EAFI_MASK2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_MASK3_HW, pEafConfig->EAFI_MASK3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_CUR_Y0_HW, pEafConfig->EAFI_CUR_Y0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CUR_Y1_HW, pEafConfig->EAFI_CUR_Y1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CUR_Y2_HW, pEafConfig->EAFI_CUR_Y2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CUR_Y3_HW, pEafConfig->EAFI_CUR_Y3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_CUR_UV0_HW, pEafConfig->EAFI_CUR_UV0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CUR_UV1_HW, pEafConfig->EAFI_CUR_UV1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CUR_UV2_HW, pEafConfig->EAFI_CUR_UV2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CUR_UV3_HW, pEafConfig->EAFI_CUR_UV3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_PRE_Y0_HW, pEafConfig->EAFI_PRE_Y0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PRE_Y1_HW, pEafConfig->EAFI_PRE_Y1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PRE_Y2_HW, pEafConfig->EAFI_PRE_Y2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PRE_Y3_HW, pEafConfig->EAFI_PRE_Y3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_PRE_UV0_HW, pEafConfig->EAFI_PRE_UV0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PRE_UV1_HW, pEafConfig->EAFI_PRE_UV1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PRE_UV2_HW, pEafConfig->EAFI_PRE_UV2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PRE_UV3_HW, pEafConfig->EAFI_PRE_UV3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_DEP0_HW, pEafConfig->EAFI_DEP0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_DEP1_HW, pEafConfig->EAFI_DEP1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_DEP2_HW, pEafConfig->EAFI_DEP2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_DEP3_HW, pEafConfig->EAFI_DEP3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_LKH_WMAP0_HW, pEafConfig->EAFI_LKH_WMAP0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_LKH_WMAP1_HW, pEafConfig->EAFI_LKH_WMAP1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_LKH_WMAP2_HW, pEafConfig->EAFI_LKH_WMAP2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_LKH_WMAP3_HW, pEafConfig->EAFI_LKH_WMAP3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_LKH_EMAP0_HW, pEafConfig->EAFI_LKH_EMAP0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_LKH_EMAP1_HW, pEafConfig->EAFI_LKH_EMAP1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_LKH_EMAP2_HW, pEafConfig->EAFI_LKH_EMAP2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_LKH_EMAP3_HW, pEafConfig->EAFI_LKH_EMAP3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_PROB0_HW, pEafConfig->EAFI_PROB0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PROB1_HW, pEafConfig->EAFI_PROB1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PROB2_HW, pEafConfig->EAFI_PROB2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_PROB3_HW, pEafConfig->EAFI_PROB3, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFO_FOUT0_HW, pEafConfig->EAFO_FOUT0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_FOUT1_HW, pEafConfig->EAFO_FOUT1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_FOUT2_HW, pEafConfig->EAFO_FOUT2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_FOUT3_HW, pEafConfig->EAFO_FOUT3, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_FOUT4_HW, pEafConfig->EAFO_FOUT4, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFO_POUT0_HW, pEafConfig->EAFO_POUT0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_POUT1_HW, pEafConfig->EAFO_POUT1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_POUT2_HW, pEafConfig->EAFO_POUT2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_POUT3_HW, pEafConfig->EAFO_POUT3, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_POUT4_HW, pEafConfig->EAFO_POUT4, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF_MASK_LB_CTL0_HW, pEafConfig->EAF_MASK_LB_CTL0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_MASK_LB_CTL1_HW, pEafConfig->EAF_MASK_LB_CTL1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_PRE_Y_LB_CTL0_HW, pEafConfig->EAF_PRE_Y_LB_CTL0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_PRE_Y_LB_CTL1_HW, pEafConfig->EAF_PRE_Y_LB_CTL1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_PRE_UV_LB_CTL0_HW, pEafConfig->EAF_PRE_UV_LB_CTL0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_PRE_UV_LB_CTL1_HW, pEafConfig->EAF_PRE_UV_LB_CTL1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_CUR_UV_LB_CTL0_HW, pEafConfig->EAF_CUR_UV_LB_CTL0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_CUR_UV_LB_CTL1_HW, pEafConfig->EAF_CUR_UV_LB_CTL1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_DMA_DCM_DIS_HW, 0x0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF_MAIN_DCM_DIS_HW, 0x0, CMDQ_REG_MASK);
/*	cmdqRecWrite(handle, EAF_DMA_DCM_ST_HW, pEafConfig->EAF_DMA_DCM_DIS, CMDQ_REG_MASK);  */
/*	cmdqRecWrite(handle, EAF_MAIN_DCM_ST_HW	, pEafConfig->EAF_DMA_DCM_DIS, CMDQ_REG_MASK); */
	cmdqRecWrite(handle, EAF_INT_CTL_HW, 0x10001, CMDQ_REG_MASK);
/*	cmdqRecWrite(handle, EAF_INT_STATUS_HW, pEafConfig->EAF_DMA_DCM_DIS, CMDQ_REG_MASK); */
/*	cmdqRecWrite(handle, EAF_SW_RST_HW, pEafConfig->EAF_DMA_DCM_DIS, CMDQ_REG_MASK);  */

	cmdqRecWrite(handle, JBFR_CFG0_HW, pEafConfig->JBFR_CFG0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG1_HW, pEafConfig->JBFR_CFG1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG2_HW, pEafConfig->JBFR_CFG2, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG3_HW, pEafConfig->JBFR_CFG3, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG4_HW, pEafConfig->JBFR_CFG4, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG5_HW, pEafConfig->JBFR_CFG5, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_SIZE_HW, pEafConfig->JBFR_SIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG6_HW, pEafConfig->JBFR_CFG6, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG7_HW, pEafConfig->JBFR_CFG7, CMDQ_REG_MASK);
	cmdqRecWrite(handle, JBFR_CFG8_HW, pEafConfig->JBFR_CFG8, CMDQ_REG_MASK);


	cmdqRecWrite(handle, SRZ6_CONTROL_HW, pEafConfig->SRZ6_CONTROL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_IN_IMG_HW, pEafConfig->SRZ6_IN_IMG, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_OUT_IMG_HW, pEafConfig->SRZ6_OUT_IMG, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_HORI_STEP_HW, pEafConfig->SRZ6_HORI_STEP, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_VERT_STEP_HW, pEafConfig->SRZ6_VERT_STEP, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_HORI_INT_OFST_HW, pEafConfig->SRZ6_HORI_INT_OFST, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_HORI_SUB_OFST_HW, pEafConfig->SRZ6_HORI_SUB_OFST, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_VERT_INT_OFST_HW, pEafConfig->SRZ6_VERT_INT_OFST, CMDQ_REG_MASK);
	cmdqRecWrite(handle, SRZ6_VERT_SUB_OFST_HW, pEafConfig->SRZ6_VERT_SUB_OFST, CMDQ_REG_MASK);


/*	cmdqRecWrite(handle, EAF_DMA_SOFT_RSTSTAT_HW, pEafConfig->EAF_DMA_SOFT_RSTSTAT, CMDQ_REG_MASK);*/
	cmdqRecWrite(handle, EAF_TDRI_BASE_ADDR_HW, pEafConfig->EAF_TDRI_BASE_ADDR, CMDQ_REG_MASK);
/*	cmdqRecWrite(handle, EAF_TDRI_OFST_ADDR_HW, pEafConfig->EAF_TDRI_OFST_ADDR, CMDQ_REG_MASK);*/
/*	cmdqRecWrite(handle, EAF_TDRI_XSIZE_HW, pEafConfig->EAF_TDRI_XSIZE, CMDQ_REG_MASK);*/
/*	cmdqRecWrite(handle, EAF_VERTICAL_FLIP_EN_HW, pEafConfig->EAF_VERTICAL_FLIP_EN, CMDQ_REG_MASK);*/
/*	cmdqRecWrite(handle, EAF_DMA_SOFT_RESET_HW, pEafConfig->EAF_DMA_SOFT_RESET, CMDQ_REG_MASK);*/
/*	cmdqRecWrite(handle, EAF_LAST_ULTRA_EN_HW, pEafConfig->EAF_LAST_ULTRA_EN, CMDQ_REG_MASK);*/
/*	cmdqRecWrite(handle, EAF_SPECIAL_FUN_EN_HW, pEafConfig->EAF_SPECIAL_FUN_EN, CMDQ_REG_MASK);*/

	cmdqRecWrite(handle, EAFO_BASE_ADDR_HW, pEafConfig->EAFO_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_OFST_ADDR_HW, pEafConfig->EAFO_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_XSIZE_HW, pEafConfig->EAFO_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_YSIZE_HW, pEafConfig->EAFO_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_STRIDE_HW, pEafConfig->EAFO_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_CON_HW, 0x80000040, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_CON2_HW, 0x200020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_CON3_HW, 0x200020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFO_CROP_HW, pEafConfig->EAFO_CROP, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAFI_BASE_ADDR_HW, pEafConfig->EAFI_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_OFST_ADDR_HW, pEafConfig->EAFI_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_XSIZE_HW, pEafConfig->EAFI_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_YSIZE_HW, pEafConfig->EAFI_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_STRIDE_HW, pEafConfig->EAFI_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CON_HW, 0x80000040, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CON2_HW, 0x200020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAFI_CON3_HW, 0x200020, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF2I_BASE_ADDR_HW, pEafConfig->EAF2I_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_OFST_ADDR_HW, pEafConfig->EAF2I_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_XSIZE_HW, pEafConfig->EAF2I_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_YSIZE_HW, pEafConfig->EAF2I_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_STRIDE_HW, pEafConfig->EAF2I_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_CON_HW, 0x80000020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_CON2_HW, 0x100010, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF2I_CON3_HW, 0x100010, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF3I_BASE_ADDR_HW, pEafConfig->EAF3I_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_OFST_ADDR_HW, pEafConfig->EAF3I_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_XSIZE_HW, pEafConfig->EAF3I_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_YSIZE_HW, pEafConfig->EAF3I_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_STRIDE_HW, pEafConfig->EAF3I_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_CON_HW, 0x80000020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_CON2_HW, 0x100010, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF3I_CON3_HW, 0x100010, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF4I_BASE_ADDR_HW, pEafConfig->EAF4I_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_OFST_ADDR_HW, pEafConfig->EAF4I_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_XSIZE_HW, pEafConfig->EAF4I_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_YSIZE_HW, pEafConfig->EAF4I_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_STRIDE_HW, pEafConfig->EAF4I_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_CON_HW, 0x80000040, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_CON2_HW, 0x200020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF4I_CON3_HW, 0x200020, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF5I_BASE_ADDR_HW, pEafConfig->EAF5I_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_OFST_ADDR_HW, pEafConfig->EAF5I_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_XSIZE_HW, pEafConfig->EAF5I_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_YSIZE_HW, pEafConfig->EAF5I_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_STRIDE_HW, pEafConfig->EAF5I_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_CON_HW, 0x80000020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_CON2_HW, 0x100010, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF5I_CON3_HW, 0x100010, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF6I_BASE_ADDR_HW, pEafConfig->EAF6I_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_OFST_ADDR_HW, pEafConfig->EAF6I_OFST_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_XSIZE_HW, pEafConfig->EAF6I_XSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_YSIZE_HW, pEafConfig->EAF6I_YSIZE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_STRIDE_HW, pEafConfig->EAF6I_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_CON_HW, 0x80000020, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_CON2_HW, 0x100010, CMDQ_REG_MASK);
	cmdqRecWrite(handle, EAF6I_CON3_HW, 0x100010, CMDQ_REG_MASK);

	cmdqRecWrite(handle, EAF_MAIN_CFG2_HW, 0x01, CMDQ_REG_MASK);

	cmdqRecWait(handle, CMDQ_EVENT_EAF_EOF);

	/* non-blocking API, Please  use cmdqRecFlushAsync() */
	cmdqRecFlushAsync(handle);
	cmdqRecReset(handle);/* if you want to re-use the handle, please reset the handle */
	cmdqRecDestroy(handle);/* recycle the memory */

#else

	EAF_WR32(EAF_MAIN_CFG0_REG, pEafConfig->EAF_MAIN_CFG0);
	EAF_WR32(EAF_MAIN_CFG1_REG, pEafConfig->EAF_MAIN_CFG1);
	EAF_WR32(EAF_MAIN_CFG2_REG, pEafConfig->EAF_MAIN_CFG2);
	EAF_WR32(EAF_MAIN_CFG3_REG, pEafConfig->EAF_MAIN_CFG3);
	EAF_WR32(EAF_HIST_CFG0_REG, pEafConfig->EAF_HIST_CFG0);
	EAF_WR32(EAF_HIST_CFG1_REG, pEafConfig->EAF_HIST_CFG1);
	EAF_WR32(EAF_SRZ_CFG0_REG, pEafConfig->EAF_SRZ_CFG0);
	EAF_WR32(EAF_BOXF_CFG0_REG, pEafConfig->EAF_BOXF_CFG0);
	EAF_WR32(EAF_DIV_CFG0_REG, pEafConfig->EAF_DIV_CFG0);
	EAF_WR32(EAF_LKHF_CFG0_REG, pEafConfig->EAF_LKHF_CFG0);
	EAF_WR32(EAF_LKHF_CFG1_REG, pEafConfig->EAF_LKHF_CFG1);

	EAF_WR32(EAFI_MASK0_REG, pEafConfig->EAFI_MASK0);
	EAF_WR32(EAFI_MASK1_REG, pEafConfig->EAFI_MASK1);
	EAF_WR32(EAFI_MASK2_REG, pEafConfig->EAFI_MASK2);
	EAF_WR32(EAFI_MASK3_REG, pEafConfig->EAFI_MASK3);

	EAF_WR32(EAFI_CUR_Y0_REG, pEafConfig->EAFI_CUR_Y0);
	EAF_WR32(EAFI_CUR_Y1_REG, pEafConfig->EAFI_CUR_Y1);
	EAF_WR32(EAFI_CUR_Y2_REG, pEafConfig->EAFI_CUR_Y2);
	EAF_WR32(EAFI_CUR_Y3_REG, pEafConfig->EAFI_CUR_Y3);

	EAF_WR32(EAFI_CUR_UV0_REG, pEafConfig->EAFI_CUR_UV0);
	EAF_WR32(EAFI_CUR_UV1_REG, pEafConfig->EAFI_CUR_UV1);
	EAF_WR32(EAFI_CUR_UV2_REG, pEafConfig->EAFI_CUR_UV2);
	EAF_WR32(EAFI_CUR_UV3_REG, pEafConfig->EAFI_CUR_UV3);

	EAF_WR32(EAFI_PRE_Y0_REG, pEafConfig->EAFI_PRE_Y0);
	EAF_WR32(EAFI_PRE_Y1_REG, pEafConfig->EAFI_PRE_Y1);
	EAF_WR32(EAFI_PRE_Y2_REG, pEafConfig->EAFI_PRE_Y2);
	EAF_WR32(EAFI_PRE_Y3_REG, pEafConfig->EAFI_PRE_Y3);

	EAF_WR32(EAFI_PRE_UV0_REG, pEafConfig->EAFI_PRE_UV0);
	EAF_WR32(EAFI_PRE_UV1_REG, pEafConfig->EAFI_PRE_UV1);
	EAF_WR32(EAFI_PRE_UV2_REG, pEafConfig->EAFI_PRE_UV2);
	EAF_WR32(EAFI_PRE_UV3_REG, pEafConfig->EAFI_PRE_UV3);

	EAF_WR32(EAFI_DEP0_REG, pEafConfig->EAFI_DEP0);
	EAF_WR32(EAFI_DEP1_REG, pEafConfig->EAFI_DEP1);
	EAF_WR32(EAFI_DEP2_REG, pEafConfig->EAFI_DEP2);
	EAF_WR32(EAFI_DEP3_REG, pEafConfig->EAFI_DEP3);

	EAF_WR32(EAFI_LKH_WMAP0_REG, pEafConfig->EAFI_LKH_WMAP0);
	EAF_WR32(EAFI_LKH_WMAP1_REG, pEafConfig->EAFI_LKH_WMAP1);
	EAF_WR32(EAFI_LKH_WMAP2_REG, pEafConfig->EAFI_LKH_WMAP2);
	EAF_WR32(EAFI_LKH_WMAP3_REG, pEafConfig->EAFI_LKH_WMAP3);

	EAF_WR32(EAFI_LKH_EMAP0_REG, pEafConfig->EAFI_LKH_EMAP0);
	EAF_WR32(EAFI_LKH_EMAP1_REG, pEafConfig->EAFI_LKH_EMAP1);
	EAF_WR32(EAFI_LKH_EMAP2_REG, pEafConfig->EAFI_LKH_EMAP2);
	EAF_WR32(EAFI_LKH_EMAP3_REG, pEafConfig->EAFI_LKH_EMAP3);

	EAF_WR32(EAFI_PROB0_REG, pEafConfig->EAFI_PROB0);
	EAF_WR32(EAFI_PROB1_REG, pEafConfig->EAFI_PROB1);
	EAF_WR32(EAFI_PROB2_REG, pEafConfig->EAFI_PROB2);
	EAF_WR32(EAFI_PROB3_REG, pEafConfig->EAFI_PROB3);

	EAF_WR32(EAFO_FOUT0_REG, pEafConfig->EAFO_FOUT0);
	EAF_WR32(EAFO_FOUT1_REG, pEafConfig->EAFO_FOUT1);
	EAF_WR32(EAFO_FOUT2_REG, pEafConfig->EAFO_FOUT2);
	EAF_WR32(EAFO_FOUT3_REG, pEafConfig->EAFO_FOUT3);
	EAF_WR32(EAFO_FOUT4_REG, pEafConfig->EAFO_FOUT4);

	EAF_WR32(EAFO_POUT0_REG, pEafConfig->EAFO_POUT0);
	EAF_WR32(EAFO_POUT1_REG, pEafConfig->EAFO_POUT1);
	EAF_WR32(EAFO_POUT2_REG, pEafConfig->EAFO_POUT2);
	EAF_WR32(EAFO_POUT3_REG, pEafConfig->EAFO_POUT3);
	EAF_WR32(EAFO_POUT4_REG, pEafConfig->EAFO_POUT4);

	EAF_WR32(EAF_MASK_LB_CTL0_REG, pEafConfig->EAF_MASK_LB_CTL0);
	EAF_WR32(EAF_MASK_LB_CTL1_REG, pEafConfig->EAF_MASK_LB_CTL1);
	EAF_WR32(EAF_PRE_Y_LB_CTL0_REG, pEafConfig->EAF_PRE_Y_LB_CTL0);
	EAF_WR32(EAF_PRE_Y_LB_CTL1_REG, pEafConfig->EAF_PRE_Y_LB_CTL1);
	EAF_WR32(EAF_PRE_UV_LB_CTL0_REG, pEafConfig->EAF_PRE_UV_LB_CTL0);
	EAF_WR32(EAF_PRE_UV_LB_CTL1_REG, pEafConfig->EAF_PRE_UV_LB_CTL1);
	EAF_WR32(EAF_CUR_UV_LB_CTL0_REG, pEafConfig->EAF_CUR_UV_LB_CTL0);
	EAF_WR32(EAF_CUR_UV_LB_CTL1_REG, pEafConfig->EAF_CUR_UV_LB_CTL1);
	EAF_WR32(EAF_DMA_DCM_DIS_REG, 0x0);
	EAF_WR32(EAF_MAIN_DCM_DIS_REG, 0x0);
/*	EAF_WR32(EAF_DMA_DCM_ST_REG, pEafConfig->EAF_DMA_DCM_DIS);*/
/*	EAF_WR32(EAF_MAIN_DCM_ST_REG	, pEafConfig->EAF_DMA_DCM_DIS);*/
	EAF_WR32(EAF_INT_CTL_REG, 0x10001);
/*	EAF_WR32(EAF_INT_STATUS_REG, pEafConfig->EAF_DMA_DCM_DIS);*/
/*	EAF_WR32(EAF_SW_RST_REG, pEafConfig->EAF_DMA_DCM_DIS);*/

	EAF_WR32(JBFR_CFG0_REG, pEafConfig->JBFR_CFG0);
	EAF_WR32(JBFR_CFG1_REG, pEafConfig->JBFR_CFG1);
	EAF_WR32(JBFR_CFG2_REG, pEafConfig->JBFR_CFG2);
	EAF_WR32(JBFR_CFG3_REG, pEafConfig->JBFR_CFG3);
	EAF_WR32(JBFR_CFG4_REG, pEafConfig->JBFR_CFG4);
	EAF_WR32(JBFR_CFG5_REG, pEafConfig->JBFR_CFG5);
	EAF_WR32(JBFR_SIZE_REG, pEafConfig->JBFR_SIZE);
	EAF_WR32(JBFR_CFG6_REG, pEafConfig->JBFR_CFG6);
	EAF_WR32(JBFR_CFG7_REG, pEafConfig->JBFR_CFG7);
	EAF_WR32(JBFR_CFG8_REG, pEafConfig->JBFR_CFG8);


	EAF_WR32(SRZ6_CONTROL_REG, pEafConfig->SRZ6_CONTROL);
	EAF_WR32(SRZ6_IN_IMG_REG, pEafConfig->SRZ6_IN_IMG);
	EAF_WR32(SRZ6_OUT_IMG_REG, pEafConfig->SRZ6_OUT_IMG);
	EAF_WR32(SRZ6_HORI_STEP_REG, pEafConfig->SRZ6_HORI_STEP);
	EAF_WR32(SRZ6_VERT_STEP_REG, pEafConfig->SRZ6_VERT_STEP);
	EAF_WR32(SRZ6_HORI_INT_OFST_REG, pEafConfig->SRZ6_HORI_INT_OFST);
	EAF_WR32(SRZ6_HORI_SUB_OFST_REG, pEafConfig->SRZ6_HORI_SUB_OFST);
	EAF_WR32(SRZ6_VERT_INT_OFST_REG, pEafConfig->SRZ6_VERT_INT_OFST);
	EAF_WR32(SRZ6_VERT_SUB_OFST_REG, pEafConfig->SRZ6_VERT_SUB_OFST);


/*	EAF_WR32(EAF_DMA_SOFT_RSTSTAT_REG, pEafConfig->EAF_DMA_SOFT_RSTSTAT);*/
	EAF_WR32(EAF_TDRI_BASE_ADDR_REG, pEafConfig->EAF_TDRI_BASE_ADDR);
/*	EAF_WR32(EAF_TDRI_OFST_ADDR_REG, pEafConfig->EAF_TDRI_OFST_ADDR);*/
/*	EAF_WR32(EAF_TDRI_XSIZE_REG, pEafConfig->EAF_TDRI_XSIZE);*/
/*	EAF_WR32(EAF_VERTICAL_FLIP_EN_REG, pEafConfig->EAF_VERTICAL_FLIP_EN);*/
/*	EAF_WR32(EAF_DMA_SOFT_RESET_REG, pEafConfig->EAF_DMA_SOFT_RESET);*/
/*	EAF_WR32(EAF_LAST_ULTRA_EN_REG, pEafConfig->EAF_LAST_ULTRA_EN);*/
/*	EAF_WR32(EAF_SPECIAL_FUN_EN_REG, pEafConfig->EAF_SPECIAL_FUN_EN);*/

	EAF_WR32(EAFO_BASE_ADDR_REG, pEafConfig->EAFO_BASE_ADDR);
	EAF_WR32(EAFO_OFST_ADDR_REG, pEafConfig->EAFO_OFST_ADDR);
	EAF_WR32(EAFO_XSIZE_REG, pEafConfig->EAFO_XSIZE);
	EAF_WR32(EAFO_YSIZE_REG, pEafConfig->EAFO_YSIZE);
	EAF_WR32(EAFO_STRIDE_REG, pEafConfig->EAFO_STRIDE);
	EAF_WR32(EAFO_CON_REG, 0x80000040);
	EAF_WR32(EAFO_CON2_REG, 0x200020);
	EAF_WR32(EAFO_CON3_REG, 0x200020);
	EAF_WR32(EAFO_CROP_REG, pEafConfig->EAFO_CROP);

	EAF_WR32(EAFI_BASE_ADDR_REG, pEafConfig->EAFI_BASE_ADDR);
	EAF_WR32(EAFI_OFST_ADDR_REG, pEafConfig->EAFI_OFST_ADDR);
	EAF_WR32(EAFI_XSIZE_REG, pEafConfig->EAFI_XSIZE);
	EAF_WR32(EAFI_YSIZE_REG, pEafConfig->EAFI_YSIZE);
	EAF_WR32(EAFI_STRIDE_REG, pEafConfig->EAFI_STRIDE);
	EAF_WR32(EAFI_CON_REG, 0x80000040);
	EAF_WR32(EAFI_CON2_REG, 0x200020);
	EAF_WR32(EAFI_CON3_REG, 0x200020);

	EAF_WR32(EAF2I_BASE_ADDR_REG, pEafConfig->EAF2I_BASE_ADDR);
	EAF_WR32(EAF2I_OFST_ADDR_REG, pEafConfig->EAF2I_OFST_ADDR);
	EAF_WR32(EAF2I_XSIZE_REG, pEafConfig->EAF2I_XSIZE);
	EAF_WR32(EAF2I_YSIZE_REG, pEafConfig->EAF2I_YSIZE);
	EAF_WR32(EAF2I_STRIDE_REG, pEafConfig->EAF2I_STRIDE);
	EAF_WR32(EAF2I_CON_REG, 0x80000020);
	EAF_WR32(EAF2I_CON2_REG, 0x100010);
	EAF_WR32(EAF2I_CON3_REG, 0x100010);

	EAF_WR32(EAF3I_BASE_ADDR_REG, pEafConfig->EAF3I_BASE_ADDR);
	EAF_WR32(EAF3I_OFST_ADDR_REG, pEafConfig->EAF3I_OFST_ADDR);
	EAF_WR32(EAF3I_XSIZE_REG, pEafConfig->EAF3I_XSIZE);
	EAF_WR32(EAF3I_YSIZE_REG, pEafConfig->EAF3I_YSIZE);
	EAF_WR32(EAF3I_STRIDE_REG, pEafConfig->EAF3I_STRIDE);
	EAF_WR32(EAF3I_CON_REG, 0x80000020);
	EAF_WR32(EAF3I_CON2_REG, 0x100010);
	EAF_WR32(EAF3I_CON3_REG, 0x100010);

	EAF_WR32(EAF4I_BASE_ADDR_REG, pEafConfig->EAF4I_BASE_ADDR);
	EAF_WR32(EAF4I_OFST_ADDR_REG, pEafConfig->EAF4I_OFST_ADDR);
	EAF_WR32(EAF4I_XSIZE_REG, pEafConfig->EAF4I_XSIZE);
	EAF_WR32(EAF4I_YSIZE_REG, pEafConfig->EAF4I_YSIZE);
	EAF_WR32(EAF4I_STRIDE_REG, pEafConfig->EAF4I_STRIDE);
	EAF_WR32(EAF4I_CON_REG, 0x80000040);
	EAF_WR32(EAF4I_CON2_REG, 0x200020);
	EAF_WR32(EAF4I_CON3_REG, 0x200020);

	EAF_WR32(EAF5I_BASE_ADDR_REG, pEafConfig->EAF5I_BASE_ADDR);
	EAF_WR32(EAF5I_OFST_ADDR_REG, pEafConfig->EAF5I_OFST_ADDR);
	EAF_WR32(EAF5I_XSIZE_REG, pEafConfig->EAF5I_XSIZE);
	EAF_WR32(EAF5I_YSIZE_REG, pEafConfig->EAF5I_YSIZE);
	EAF_WR32(EAF5I_STRIDE_REG, pEafConfig->EAF5I_STRIDE);
	EAF_WR32(EAF5I_CON_REG, 0x80000020);
	EAF_WR32(EAF5I_CON2_REG, 0x100010);
	EAF_WR32(EAF5I_CON3_REG, 0x100010);

	EAF_WR32(EAF6I_BASE_ADDR_REG, pEafConfig->EAF6I_BASE_ADDR);
	EAF_WR32(EAF6I_OFST_ADDR_REG, pEafConfig->EAF6I_OFST_ADDR);
	EAF_WR32(EAF6I_XSIZE_REG, pEafConfig->EAF6I_XSIZE);
	EAF_WR32(EAF6I_YSIZE_REG, pEafConfig->EAF6I_YSIZE);
	EAF_WR32(EAF6I_STRIDE_REG, pEafConfig->EAF6I_STRIDE);
	EAF_WR32(EAF6I_CON_REG, 0x80000020);
	EAF_WR32(EAF6I_CON2_REG, 0x100010);
	EAF_WR32(EAF6I_CON3_REG, 0x100010);

	EAF_WR32(EAF_MAIN_CFG2_REG, 0x01);


#endif

	return 0;
}


#define EAF_IS_BUSY    0x2
#define BYPASS_REG (0)




/*
 *
 */
static MINT32 EAF_DumpReg(void)
{
	MINT32 Ret = 0;
	int i, j;

	/*  */
	LOG_INF("- E.");
	/*  */
	LOG_INF("EAF Debug Info\n");
/*	LOG_INF("[0x%08X %08X]\n", (unsigned int)(EAF_CTRL_HW),(unsigned int)RSC_RD32(RSC_CTRL_REG));*/

	for (i = 0; i < _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_; i++) {
		LOG_INF
			("EAF Req:State:%d, procID:0x%08X, callerID:0x%08X, enqReqNum:%d, FraWRIdx:%d, FraRDIdx:%d\n",
		     g_EAF_ReqRing.EAFReq_Struct[i].State, g_EAF_ReqRing.EAFReq_Struct[i].processID,
		     g_EAF_ReqRing.EAFReq_Struct[i].callerID, g_EAF_ReqRing.EAFReq_Struct[i].enqueReqNum,
		     g_EAF_ReqRing.EAFReq_Struct[i].FrameWRIdx, g_EAF_ReqRing.EAFReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_EAF_FRAME_REQUEST_;) {
			LOG_INF
			    ("EAF:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
			     j, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j]
			     , j + 1, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j + 1],
			     j + 2, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j + 2]
			     , j + 3, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j + 3]);
			j = j + 4;
		}

	}

	LOG_INF("- X.");
	/*  */
	return Ret;
}


#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
static inline void EAF_Prepare_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> EAF clk */
	ret = clk_prepare(eaf_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot prepare CG_SCP_SYS_MM0 clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_2X clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_GALS_M0_2X clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_GALS_M1_2X clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_UPSZ0 clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_UPSZ1 clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_FIFO0 clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_SMI_COMMON_FIFO1 clock\n");

	ret = clk_prepare(eaf_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot prepare CG_MM_LARB5 clock\n");

	ret = clk_prepare(eaf_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot prepare CG_SCP_SYS_ISP clock\n");

	ret = clk_prepare(eaf_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot prepare CG_IMGSYS_LARB clock\n");

	ret = clk_prepare(eaf_clk.CG_IMGSYS_EAF);
	if (ret)
		LOG_ERR("cannot prepare CG_IMGSYS_EAF clock\n");

}

static inline void EAF_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> EAF  clk */
	ret = clk_enable(eaf_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot enable CG_SCP_SYS_MM0 clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_2X clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_GALS_M0_2X clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_GALS_M1_2X clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_UPSZ0 clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_UPSZ1 clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_FIFO0 clock\n");

	ret = clk_enable(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("cannot enable CG_MM_SMI_COMMON_FIFO1 clock\n");

	ret = clk_enable(eaf_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot enable CG_MM_LARB5 clock\n");


	ret = clk_enable(eaf_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot enable CG_SCP_SYS_ISP clock\n");

	ret = clk_enable(eaf_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot enable CG_IMGSYS_LARB clock\n");

	ret = clk_enable(eaf_clk.CG_IMGSYS_EAF);
	if (ret)
		LOG_ERR("cannot enable CG_IMGSYS_EAF clock\n");

}

static inline void EAF_Prepare_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> EAF clk */
#ifndef SMI_CLK
	ret = clk_prepare_enable(eaf_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_SCP_SYS_MM0 clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_2X clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_GALS_M0_2X clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_GALS_M1_2X clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_UPSZ0 clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_UPSZ1 clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_FIFO0 clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_FIFO1 clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_LARB5 clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_SCP_SYS_ISP clock\n");

	ret = clk_prepare_enable(eaf_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_LARB clock\n");
#else
	smi_bus_enable(SMI_LARB_IMGSYS1, "camera_eaf");
#endif

	ret = clk_prepare_enable(eaf_clk.CG_IMGSYS_EAF);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_EAF clock\n");

}

static inline void EAF_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: EAF clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0 */
	clk_unprepare(eaf_clk.CG_IMGSYS_EAF);
	clk_unprepare(eaf_clk.CG_IMGSYS_LARB);
	clk_unprepare(eaf_clk.CG_SCP_SYS_ISP);
	clk_unprepare(eaf_clk.CG_MM_LARB5);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON_2X);
	clk_unprepare(eaf_clk.CG_MM_SMI_COMMON);
	clk_unprepare(eaf_clk.CG_SCP_SYS_MM0);
}

static inline void EAF_Disable_ccf_clock(void)
{
	/* must keep this clk close order: EAF clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0 */
	clk_disable(eaf_clk.CG_IMGSYS_EAF);
	clk_disable(eaf_clk.CG_IMGSYS_LARB);
	clk_disable(eaf_clk.CG_SCP_SYS_ISP);
	clk_disable(eaf_clk.CG_MM_LARB5);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON_2X);
	clk_disable(eaf_clk.CG_MM_SMI_COMMON);
	clk_disable(eaf_clk.CG_SCP_SYS_MM0);
}

static inline void EAF_Disable_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: EAF clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0 */
	clk_disable_unprepare(eaf_clk.CG_IMGSYS_EAF);
#ifndef SMI_CLK
	clk_disable_unprepare(eaf_clk.CG_IMGSYS_LARB);
	clk_disable_unprepare(eaf_clk.CG_SCP_SYS_ISP);
	clk_disable_unprepare(eaf_clk.CG_MM_LARB5);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON_2X);
	clk_disable_unprepare(eaf_clk.CG_MM_SMI_COMMON);
	clk_disable_unprepare(eaf_clk.CG_SCP_SYS_MM0);
#else
	smi_bus_disable(SMI_LARB_IMGSYS1, "camera_eaf");
#endif
}
#endif

/*******************************************************************************
*
********************************************************************************/
static void EAF_EnableClock(MBOOL En)
{

	LOG_INF("EAF_EnableClock\n");

	if (En) {		/* Enable clock. */
		/* LOG_DBG("EAF clock enbled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		switch (g_u4EnableClockCount) {
		case 0:
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#ifndef __EAF_EP_NO_CLKMGR__

			EAF_Prepare_Enable_ccf_clock();
#else
			/* Enable clock by hardcode:
			* 1. CAMSYS_CG_CLR (0x1A000008) = 0xffffffff;
			* 2. IMG_CG_CLR (0x15000008) = 0xffffffff;
			*/
			setReg = 0xFFFFFFFF;
			EAF_WR32(IMGSYS_REG_CG_CLR, setReg);
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
		spin_lock(&(EAFInfo.SpinLockEAF));
		g_u4EnableClockCount++;
		spin_unlock(&(EAFInfo.SpinLockEAF));
	} else {		/* Disable clock. */

		/* LOG_DBG("EAF clock disabled. g_u4EnableClockCount: %d.", g_u4EnableClockCount); */
		spin_lock(&(EAFInfo.SpinLockEAF));
		g_u4EnableClockCount--;
		spin_unlock(&(EAFInfo.SpinLockEAF));
		switch (g_u4EnableClockCount) {
		case 0:
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#ifndef __EAF_EP_NO_CLKMGR__
			EAF_Disable_Unprepare_ccf_clock();
#else
			/* Disable clock by hardcode:
			* 1. CAMSYS_CG_SET (0x1A000004) = 0xffffffff;
			* 2. IMG_CG_SET (0x15000004) = 0xffffffff;
			*/
			setReg = 0xFFFFFFFF;
			EAF_WR32(IMGSYS_REG_CG_SET, setReg);
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
static inline void EAF_Reset(void)
{

	LOG_INF(" EAF Reset start!\n");
	spin_lock(&(EAFInfo.SpinLockEAFRef));

	if (EAFInfo.UserCount > 1) {
		spin_unlock(&(EAFInfo.SpinLockEAFRef));
		LOG_DBG("Curr UserCount(%d) users exist", EAFInfo.UserCount);
	} else {
		spin_unlock(&(EAFInfo.SpinLockEAFRef));

		/* Reset EAF flow */
#if 0
	Software Reset Flow
	¡EStep1 ¡V set EAF_DMA_STOP = 1, EAF_RST_BIT = 0
	¡EStep2 ¡V wait until EAF_DMA_STOP_STATUS = 1
	¡EStep3 ¡V set EAF_DMA_STOP = 1, EAF_RST_BIT = 1
	¡EStep4 ¡V set EAF_DMA_STOP = 0, EAF_RST_BIT = 1
	¡EStep5 ¡V set EAF_DMA_STOP = 0, EAF_RST_BIT = 0
#endif

	    EAF_WR32(EAF_MAIN_DCM_DIS_REG, 0xffffffff);

	    EAF_WR32(EAF_SW_RST_REG, 0x1);

		while ((EAF_RD32(EAF_SW_RST_REG) & 0x02) != 0x2)
			LOG_DBG("EAF resetting...\n");

	    EAF_WR32(EAF_SW_RST_REG, 0x101);
	    EAF_WR32(EAF_SW_RST_REG, 0x100);
	    EAF_WR32(EAF_SW_RST_REG, 0x0);

	    EAF_WR32(EAF_MAIN_DCM_DIS_REG, 0x0);

	    LOG_DBG(" EAF Reset end!\n");
	}

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_ReadReg(EAF_REG_IO_STRUCT *pRegIo)
{
	MUINT32 i;
	MINT32 Ret = 0;
	/*  */
	EAF_REG_STRUCT reg;
	/* MUINT32* pData = (MUINT32*)pRegIo->Data; */
	EAF_REG_STRUCT *pData = (EAF_REG_STRUCT *) pRegIo->pData;

	for (i = 0; i < pRegIo->Count; i++) {
		if (get_user(reg.Addr, (MUINT32 *) &pData->Addr) != 0) {
			LOG_ERR("get_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		/*  */
		if ((ISP_EAF_BASE + reg.Addr >= ISP_EAF_BASE) &&
			(ISP_EAF_BASE + reg.Addr < (ISP_EAF_BASE + EAF_REG_RANGE))) {
			reg.Val = EAF_RD32(ISP_EAF_BASE + reg.Addr);
		} else {
			LOG_ERR("Wrong eaf address(0x%p)", (ISP_EAF_BASE + reg.Addr));
			reg.Val = 0;
		}
		/*  */
		/* printk("[KernelRDReg]addr(0x%x),value()0x%x\n",EAF_ADDR_CAMINF + reg.Addr,reg.Val); */

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
static MINT32 EAF_WriteRegToHw(EAF_REG_STRUCT *pReg, MUINT32 Count)
{
	MINT32 Ret = 0;
	MUINT32 i;
	MBOOL dbgWriteReg;

	/* Use local variable to store EAFInfo.DebugMask & EAF_DBG_WRITE_REG for saving lock time */
	spin_lock(&(EAFInfo.SpinLockEAF));
	dbgWriteReg = EAFInfo.DebugMask & EAF_DBG_WRITE_REG;
	spin_unlock(&(EAFInfo.SpinLockEAF));

	/*  */
	if (dbgWriteReg)
		LOG_DBG("- E.\n");

	/*  */
	for (i = 0; i < Count; i++) {
		if (dbgWriteReg) {
			LOG_DBG("Addr(0x%lx), Val(0x%x)\n",
				(unsigned long)(ISP_EAF_BASE + pReg[i].Addr),
				(MUINT32) (pReg[i].Val));
		}

		if (((ISP_EAF_BASE + pReg[i].Addr) < (ISP_EAF_BASE + EAF_REG_RANGE))) {
			EAF_WR32(ISP_EAF_BASE + pReg[i].Addr, pReg[i].Val);
		} else {
			LOG_ERR("wrong eaf address(0x%lx)\n",
				(unsigned long)(ISP_EAF_BASE + pReg[i].Addr));
		}
	}

	/*  */
	return Ret;
}



/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_WriteReg(EAF_REG_IO_STRUCT *pRegIo)
{
	MINT32 Ret = 0;
	/**
	**   MINT32 TimeVd = 0;
	**   MINT32 TimeExpdone = 0;
	**   MINT32 TimeTasklet = 0;
	**/
	/* MUINT8* pData = NULL; */
	EAF_REG_STRUCT *pData = NULL;
	/*  */
	if (EAFInfo.DebugMask & EAF_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n", (pRegIo->pData), (pRegIo->Count));

	/* pData = (MUINT8*)kmalloc((pRegIo->Count)*sizeof(EAF_REG_STRUCT), GFP_ATOMIC); */
	pData = kmalloc((pRegIo->Count) * sizeof(EAF_REG_STRUCT), GFP_ATOMIC);
	if (pData == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)\n", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	}
	/*  */
	if ((pRegIo->pData == NULL) || (pRegIo->Count == 0)) {
		LOG_ERR("ERROR: pRegIo->pData is NULL or Count:%d\n", pRegIo->Count);
		Ret = -EFAULT;
		goto EXIT;
	}
	if (copy_from_user
	    (pData, (void __user *)(pRegIo->pData), pRegIo->Count * sizeof(EAF_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = EAF_WriteRegToHw(pData, pRegIo->Count);
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
static MINT32 EAF_WaitIrq(EAF_WAIT_IRQ_STRUCT *WaitIrq)
{

	MINT32 Ret = 0;
	MINT32 Timeout = WaitIrq->Timeout;
	EAF_PROCESS_ID_ENUM whichReq = EAF_PROCESS_ID_NONE;

	/*MUINT32 i; */
	unsigned long flags;	/* old: MUINT32 flags; *//* FIX to avoid build warning */
	MUINT32 irqStatus;
	/*int cnt = 0; */
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
	if (EAFInfo.DebugMask & EAF_DBG_INT) {
		if (WaitIrq->Status & EAFInfo.IrqInfo.Mask[WaitIrq->Type]) {
			if (WaitIrq->UserKey > 0) {
				LOG_DBG("+WaitIrq Clr(%d),Type(%d)\n", WaitIrq->Clear, WaitIrq->Type);
				LOG_DBG("+WaitIrq Status(0x%08X)\n", WaitIrq->Status);
				LOG_DBG("+WaitIrq Timeout(%d),user(%d)\n", WaitIrq->Timeout, WaitIrq->UserKey);
				LOG_DBG("+WaitIrq ProcessID(%d)\n", WaitIrq->ProcessID);
			}
		}
	}


	/* 1. wait type update */
	if (WaitIrq->Clear == EAF_IRQ_CLEAR_STATUS) {
		spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		/* LOG_DBG("WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been cleared"
		*,WaitIrq->EventInfo.Clear,WaitIrq->Type,EAFInfo.IrqInfo.Status[WaitIrq->Type]);
		*/
		/* EAFInfo.IrqInfo.Status[WaitIrq->Type][WaitIrq->EventInfo.UserKey] &=
		*   (~WaitIrq->EventInfo.Status);
		*/
		EAFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
		spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		return Ret;
	}

	if (WaitIrq->Clear == EAF_IRQ_CLEAR_WAIT) {
		spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		if (EAFInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
			EAFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);

		spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
	} else if (WaitIrq->Clear == EAF_IRQ_CLEAR_ALL) {
		spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);

		EAFInfo.IrqInfo.Status[WaitIrq->Type] = 0;
		spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
	}
	/* EAF_IRQ_WAIT_CLEAR ==> do nothing */


	/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
	spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
	irqStatus = EAFInfo.IrqInfo.Status[WaitIrq->Type];
	spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);

	if (WaitIrq->Status & EAF_INT_ST) {
		whichReq = EAF_PROCESS_ID_EAF;
	} else {
		LOG_ERR("No Such Stats can be waited!! irq Type/User/Sts/Pid(0x%x/%d/0x%x/%d)\n",
			WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, WaitIrq->ProcessID);
	}


#ifdef EAF_WAITIRQ_LOG
	LOG_INF("bef wait_event: Timeout(%d),Clr(%d),Type(%d)\n", WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type);
	LOG_INF("bef wait_event: IrqStatus(0x%08X),WaitStatus(0x%08X)\n", irqStatus, WaitIrq->Status);
	LOG_INF("bef wait_event: userKey(%d), ProcessID(%d)\n", WaitIrq->UserKey, WaitIrq->ProcessID);

#endif

	/* 2. start to wait signal */
	Timeout = wait_event_interruptible_timeout(EAFInfo.WaitQueueHead,
						   EAF_GetIRQState(WaitIrq->Type, WaitIrq->UserKey,
								   WaitIrq->Status, whichReq, WaitIrq->ProcessID),
						   EAF_MsToJiffies(WaitIrq->Timeout));

	/* check if user is interrupted by system signal */
	if ((Timeout != 0) &&
	    (!EAF_GetIRQState(WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status, whichReq, WaitIrq->ProcessID))) {
		LOG_DBG("interrupted by system signal,return value(%d)\n", Timeout);
		LOG_DBG("irq Type/User(0x%x/%d)\n", WaitIrq->Type, WaitIrq->UserKey);
		LOG_DBG("irq Sts/Pid(0x%x/%d)\n", WaitIrq->Status, WaitIrq->ProcessID);
		Ret = -ERESTARTSYS;	/* actually it should be -ERESTARTSYS */
		goto EXIT;
	}
	/* timeout */
	if (Timeout == 0) {
		/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
		spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = EAFInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);

		LOG_ERR("Timeout!!! ERRRR WaitIrq Timeout(%d) Clear(%d)\n", WaitIrq->Timeout, WaitIrq->Clear);
		LOG_ERR("Type(%d), IrqStatus(0x%08X), WaitStatus(0x%08X)\n", WaitIrq->Type, irqStatus, WaitIrq->Status);
		LOG_ERR("userKey(%d), ProcessID(%d)\n", WaitIrq->UserKey, WaitIrq->ProcessID);


		if (WaitIrq->bDumpReg)
			EAF_DumpReg();

		Ret = -EFAULT;
		goto EXIT;
	} else {
		/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
#ifdef __EAF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("EAF_WaitIrq");
#endif

		spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = EAFInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);

		if (WaitIrq->Clear == EAF_IRQ_WAIT_CLEAR) {
			spin_lock_irqsave(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);

#ifdef EAF_MULTIPROCESS_TIMEING_ISSUE
			EAFInfo.ReadReqIdx =
			    (EAFInfo.ReadReqIdx + 1) % _SUPPORT_MAX_EAF_FRAME_REQUEST_;
			/* actually, it doesn't happen the timging issue!! */
			/* wake_up_interruptible(&EAFInfo.WaitQueueHead); */
#endif
			if (WaitIrq->Status & EAF_INT_ST) {
				EAFInfo.IrqInfo.EafIrqCnt--;
				if (EAFInfo.IrqInfo.EafIrqCnt == 0)
					EAFInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
			} else {
				LOG_ERR("EAF_IRQ_WAIT_CLEAR Error, Type(%d), WaitStatus(0x%08X)",
					WaitIrq->Type, WaitIrq->Status);
			}
			spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[WaitIrq->Type]), flags);
		}

#ifdef EAF_WAITIRQ_LOG
		LOG_INF("no Timeout!!!: WaitIrq Timeout(%d) Clear(%d), Type(%d), IrqStatus(0x%08X)\n",
			 WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus);
		LOG_INF("no Timeout!!!: WaitStatus(0x%08X), Timeout(%d),userKey(%d),whichReq(%d),ProcessID(%d)\n",
			 WaitIrq->Status, WaitIrq->Timeout, WaitIrq->UserKey, whichReq, WaitIrq->ProcessID);
		LOG_INF("no Timeout!!!: EafIrqCnt(0x%08X), WriteReqIdx(0x%08X), ReadReqIdx(0x%08X)\n",
			 EAFInfo.IrqInfo.EafIrqCnt, EAFInfo.WriteReqIdx, EAFInfo.ReadReqIdx);
#endif

#ifdef __EAF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif

	}


EXIT:


	return Ret;
}


/*******************************************************************************
*
********************************************************************************/
static long EAF_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	MINT32 Ret = 0;

	/*MUINT32 pid = 0; */
	EAF_REG_IO_STRUCT RegIo;
	EAF_WAIT_IRQ_STRUCT IrqInfo;
	EAF_CLEAR_IRQ_STRUCT ClearIrq;
	EAF_USER_INFO_STRUCT *pUserInfo;
	EAF_Config eaf_EafConfig;
	EAF_Request eaf_EafReq;
	MINT32 EafWriteIdx = 0;
	int idx;
	int enqueNum;
	int dequeNum;
	unsigned long flags; /* old: unsigned int flags;*//* FIX to avoid build warning */

	LOG_INF("EAF_ioctl Cmd(%d)\n", Cmd);


	/*  */
	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		return -EFAULT;
	}
	/*  */
	pUserInfo = (EAF_USER_INFO_STRUCT *) (pFile->private_data);
	/*  */
	switch (Cmd) {
	case EAF_RESET:
		{
			spin_lock(&(EAFInfo.SpinLockEAF));
			EAF_Reset();
			spin_unlock(&(EAFInfo.SpinLockEAF));
			break;
		}

		/*  */
	case EAF_DUMP_REG:
		{
			Ret = EAF_DumpReg();
			break;
		}
	case EAF_DUMP_ISR_LOG:
		{
			MUINT32 currentPPB = m_CurrentPPB;

			spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
			m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
			spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]),
					       flags);

			IRQ_LOG_PRINTER(EAF_IRQ_TYPE_INT_EAF_ST, currentPPB, _LOG_INF);
			IRQ_LOG_PRINTER(EAF_IRQ_TYPE_INT_EAF_ST, currentPPB, _LOG_ERR);
			break;
		}
	case EAF_READ_REGISTER:
		{
			if (copy_from_user(&RegIo, (void *)Param, sizeof(EAF_REG_IO_STRUCT)) == 0) {
				/* 2nd layer behavoir of copy from user is implemented in EAF_ReadReg(...) */
				Ret = EAF_ReadReg(&RegIo);
			} else {
				LOG_ERR("EAF_READ_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case EAF_WRITE_REGISTER:
		{
			if (copy_from_user(&RegIo, (void *)Param, sizeof(EAF_REG_IO_STRUCT)) == 0) {
				/* 2nd layer behavoir of copy from user is implemented in EAF_WriteReg(...) */
				Ret = EAF_WriteReg(&RegIo);
			} else {
				LOG_ERR("EAF_WRITE_REGISTER copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case EAF_WAIT_IRQ:
		{
			if (copy_from_user(&IrqInfo, (void *)Param, sizeof(EAF_WAIT_IRQ_STRUCT)) == 0) {
				/*  */
				if ((IrqInfo.Type >= EAF_IRQ_TYPE_AMOUNT) || (IrqInfo.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)", IrqInfo.Type);
					goto EXIT;
				}

				if ((IrqInfo.UserKey >= IRQ_USER_NUM_MAX) || (IrqInfo.UserKey < 0)) {
					LOG_ERR("invalid userKey(%d), max(%d), force userkey = 0\n",
						IrqInfo.UserKey, IRQ_USER_NUM_MAX);
					IrqInfo.UserKey = 0;
				}

				LOG_INF("IRQ clear(%d), type(%d), userKey(%d), timeout(%d), status(%d)\n",
				     IrqInfo.Clear, IrqInfo.Type, IrqInfo.UserKey, IrqInfo.Timeout, IrqInfo.Status);

				IrqInfo.ProcessID = pUserInfo->Pid;
				Ret = EAF_WaitIrq(&IrqInfo);

				if (copy_to_user((void *)Param, &IrqInfo, sizeof(EAF_WAIT_IRQ_STRUCT)) != 0) {
					LOG_ERR("copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("EAF_WAIT_IRQ copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case EAF_CLEAR_IRQ:
		{
			if (copy_from_user(&ClearIrq, (void *)Param, sizeof(EAF_CLEAR_IRQ_STRUCT))
			    == 0) {
				LOG_DBG("EAF_CLEAR_IRQ Type(%d)", ClearIrq.Type);

				if ((ClearIrq.Type >= EAF_IRQ_TYPE_AMOUNT) || (ClearIrq.Type < 0)) {
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

				LOG_DBG("EAF_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)\n",
					ClearIrq.Type, ClearIrq.Status,
					EAFInfo.IrqInfo.Status[ClearIrq.Type]);
				spin_lock_irqsave(&(EAFInfo.SpinLockIrq[ClearIrq.Type]), flags);
				EAFInfo.IrqInfo.Status[ClearIrq.Type] &= (~ClearIrq.Status);
				spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[ClearIrq.Type]),
						       flags);
			} else {
				LOG_ERR("EAF_CLEAR_IRQ copy_from_user failed\n");
				Ret = -EFAULT;
			}
			break;
		}
	case EAF_ENQNUE_NUM:
		{
			/* enqueNum */
			if (copy_from_user(&enqueNum, (void *)Param, sizeof(int)) == 0) {
				if (EAF_REQUEST_STATE_EMPTY ==
				    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].
				    State) {
					spin_lock_irqsave(&
							  (EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].processID = pUserInfo->Pid;
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].enqueReqNum = enqueNum;
					spin_unlock_irqrestore(&
							       (EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
					if (enqueNum > _SUPPORT_MAX_EAF_FRAME_REQUEST_) {
						LOG_ERR("EAF Enque Num is bigger than enqueNum:%d\n", enqueNum);
						LOG_ERR("EAF Enque Num is bigger than enqueNum:%d\n", enqueNum);
					}
					LOG_DBG("EAF_ENQNUE_NUM:%d\n", enqueNum);
				} else {
					LOG_ERR
					    ("WFME Enque request state is not empty:%d, writeIdx:%d, readIdx:%d\n",
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									       WriteIdx].State, g_EAF_ReqRing.WriteIdx,
					     g_EAF_ReqRing.ReadIdx);
				}
			} else {
				LOG_ERR("EAF_EQNUE_NUM copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
		/* EAF_Config */
	case EAF_ENQUE:
		{
			if (copy_from_user(&eaf_EafConfig, (void *)Param, sizeof(EAF_Config)) == 0) {
				/* LOG_DBG("EAF_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)",
				* ClearIrq.Type, ClearIrq.Status, EAFInfo.IrqInfo.Status[ClearIrq.Type]);
				*/
				spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]),
						  flags);
				if ((g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].State
					== EAF_REQUEST_STATE_EMPTY)
				    && (g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].FrameWRIdx <
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].enqueReqNum)) {
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].
					    EafFrameStatus[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									   WriteIdx].FrameWRIdx]
										= EAF_FRAME_STATUS_ENQUE;
					memcpy(&g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].
					       EafFrameConfig[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									      WriteIdx].FrameWRIdx++], &eaf_EafConfig,
					       sizeof(EAF_Config));
					if (g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].FrameWRIdx ==
					    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].enqueReqNum) {
						g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].State
							= EAF_REQUEST_STATE_PENDING;
						g_EAF_ReqRing.WriteIdx = (g_EAF_ReqRing.WriteIdx + 1)
							% _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
						LOG_DBG("EAF enque done!!\n");
					} else {
						LOG_DBG("EAF enque frame!!\n");
					}
				} else {
					LOG_ERR("NoEmptyEAFBuf! WriteIdx(%d),State(%d),FraWRIdx(%d),enqReqNum(%d)\n",
					     g_EAF_ReqRing.WriteIdx,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].State,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].FrameWRIdx,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].enqueReqNum);
				}

				spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
				LOG_DBG("ConfigEAF!!\n");
				ConfigEAF();

			} else {
				LOG_ERR("EAF_EAF_ENQUE copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	case EAF_ENQUE_REQ:
		{
			if (copy_from_user(&eaf_EafReq, (void *)Param, sizeof(EAF_Request)) == 0) {
				LOG_DBG("EAF_ENQNUE_NUM:%d, pid:%d\n", eaf_EafReq.m_ReqNum, pUserInfo->Pid);
				if (eaf_EafReq.m_ReqNum > _SUPPORT_MAX_EAF_FRAME_REQUEST_) {
					LOG_ERR("EAF Enque Num is bigger than enqueNum:%d\n", eaf_EafReq.m_ReqNum);
					Ret = -EFAULT;
					goto EXIT;
				}
				if (copy_from_user
				    (g_EafEnqueReq_Struct.EafFrameConfig,
				     (void *)eaf_EafReq.m_pEafConfig,
				     eaf_EafReq.m_ReqNum * sizeof(EAF_Config)) != 0) {
					LOG_ERR("copy EAFConfig from request is fail!!\n");
					Ret = -EFAULT;
					goto EXIT;
				}

				mutex_lock(&gEafMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]),
						  flags);
				if (g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].State
					== EAF_REQUEST_STATE_EMPTY) {
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].processID = pUserInfo->Pid;
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].enqueReqNum
						= eaf_EafReq.m_ReqNum;

					for (idx = 0; idx < eaf_EafReq.m_ReqNum; idx++) {
						g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].
						    EafFrameStatus[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].
								    FrameWRIdx] = EAF_FRAME_STATUS_ENQUE;
						memcpy(&g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].
						       EafFrameConfig[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									WriteIdx].FrameWRIdx++],
						       &g_EafEnqueReq_Struct.EafFrameConfig[idx], sizeof(EAF_Config));
					}
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].State
						= EAF_REQUEST_STATE_PENDING;
					EafWriteIdx = g_EAF_ReqRing.WriteIdx;
					g_EAF_ReqRing.WriteIdx = (g_EAF_ReqRing.WriteIdx + 1)
						% _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
					LOG_DBG("EAF request enque done!!\n");
				} else {
					LOG_ERR("NoEmptyEAFBuf! WriteIdx(%d),State(%d),FraWRIdx(%d),enqReqNum(%d)\n",
					     g_EAF_ReqRing.WriteIdx,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].State,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].FrameWRIdx,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.WriteIdx].enqueReqNum);
				}
				spin_unlock_irqrestore(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
				LOG_DBG("ConfigEAF Request!!\n");
				ConfigEAFRequest(EafWriteIdx);

				mutex_unlock(&gEafMutex);
			} else {
				LOG_ERR("EAF_ENQUE_REQ copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	case EAF_DEQUE_NUM:
		{
			if (EAF_REQUEST_STATE_FINISHED ==
			    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State) {
				dequeNum =
				    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum;
				LOG_DBG("EAF_DEQUE_NUM(%d)\n", dequeNum);
			} else {
				dequeNum = 0;
				LOG_ERR("DeqNum:NoAnyEAFReadyBuf!!ReadIdx(%d),State(%d),FraRDIdx(%d),enqReqNum(%d)\n",
				     g_EAF_ReqRing.ReadIdx,
				     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State,
				     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx,
				     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum);
			}
			if (copy_to_user((void *)Param, &dequeNum, sizeof(MUINT32)) != 0) {
				LOG_ERR("EAF_EAF_DEQUE_NUM copy_to_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}

	case EAF_DEQUE:
		{
			spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
			if ((EAF_REQUEST_STATE_FINISHED ==
			     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
			     State)
			    && (g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
				FrameRDIdx <
				g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
				enqueReqNum)) {
				/* dequeNum = g_DVE_RequestRing.DVEReq_Struct[g_DVE_RequestRing.ReadIdx].enqueReqNum; */
				if (EAF_FRAME_STATUS_FINISHED ==
				    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
				    EafFrameStatus[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
						    FrameRDIdx]) {

					memcpy(&eaf_EafConfig,
					       &g_EAF_ReqRing.
					       EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
					       EafFrameConfig[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									      ReadIdx].FrameRDIdx],
					       sizeof(EAF_Config));
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									  ReadIdx].EafFrameStatus[g_EAF_ReqRing.
							    EAFReq_Struct[g_EAF_ReqRing.
									   ReadIdx].FrameRDIdx++] =
					    EAF_FRAME_STATUS_EMPTY;
				}
				if (g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx ==
				    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum) {
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State =
					    EAF_REQUEST_STATE_EMPTY;
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameWRIdx = 0;
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx = 0;
					g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum = 0;
					g_EAF_ReqRing.ReadIdx = (g_EAF_ReqRing.ReadIdx + 1)
						% _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
					LOG_DBG("EAF ReadIdx(%d)\n", g_EAF_ReqRing.ReadIdx);
				}
				spin_unlock_irqrestore(&
						       (EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]),
						       flags);
				if (copy_to_user
				    ((void *)Param,
				     &g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
				     EafFrameConfig[g_EAF_ReqRing.
						     EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
						     FrameRDIdx], sizeof(EAF_Config)) != 0) {
					LOG_ERR("EAF_EAF_DEQUE copy_to_user failed\n");
					Ret = -EFAULT;
				}

			} else {
				spin_unlock_irqrestore(&
						       (EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);
				LOG_ERR("No Any EAF Ready Buf!!, ReadIdx(%d), State(%d), FraRDIdx(%d), enqNum(%d)\n",
				     g_EAF_ReqRing.ReadIdx,
				     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State,
				     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx,
				     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum);
			}

			break;
		}
	case EAF_DEQUE_REQ:
		{
			if (copy_from_user(&eaf_EafReq, (void *)Param, sizeof(EAF_Request)) == 0) {
				mutex_lock(&gEafDequeMutex);	/* Protect the Multi Process */

				spin_lock_irqsave(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]), flags);

				if (EAF_REQUEST_STATE_FINISHED ==
				    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State) {
					dequeNum = g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									      ReadIdx].enqueReqNum;
					LOG_DBG("EAF_DEQUE_NUM(%d)\n", dequeNum);
				} else {
					dequeNum = 0;
					LOG_ERR("DeqNum:NoAnyRdyBuf!!RdIdx(%d),Sta(%d),FraRDIdx(%d),enqReqNum(%d)\n",
					     g_EAF_ReqRing.ReadIdx,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx,
					     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum);
				}
				eaf_EafReq.m_ReqNum = dequeNum;

				for (idx = 0; idx < dequeNum; idx++) {
					if (EAF_FRAME_STATUS_FINISHED ==
					    g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
									      ReadIdx].
						EafFrameStatus[g_EAF_ReqRing.EAFReq_Struct
							[g_EAF_ReqRing.ReadIdx].FrameRDIdx]) {

						memcpy(&g_EafDequeReq_Struct.EafFrameConfig[idx],
						       &g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
						       EafFrameConfig[g_EAF_ReqRing.EAFReq_Struct
								       [g_EAF_ReqRing.ReadIdx].
								       FrameRDIdx],
						       sizeof(EAF_Config));
						g_EAF_ReqRing.
						    EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
						    EafFrameStatus[g_EAF_ReqRing.EAFReq_Struct
								    [g_EAF_ReqRing.ReadIdx].
								    FrameRDIdx++] =
						    EAF_FRAME_STATUS_EMPTY;
					} else {
						LOG_ERR("Err!id(%d),deqNum(%d),RdId(%d),FraRDId(%d),EafFraSta(%d)\n",
						     idx, dequeNum, g_EAF_ReqRing.ReadIdx,
						     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx,
						     g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].
						     EafFrameStatus[g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.
						     ReadIdx].FrameRDIdx]);
					}
				}
				g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].State = EAF_REQUEST_STATE_EMPTY;
				g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameWRIdx = 0;
				g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].FrameRDIdx = 0;
				g_EAF_ReqRing.EAFReq_Struct[g_EAF_ReqRing.ReadIdx].enqueReqNum = 0;
				g_EAF_ReqRing.ReadIdx = (g_EAF_ReqRing.ReadIdx + 1)
					% _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_;
				LOG_DBG("EAF Request ReadIdx(%d)\n", g_EAF_ReqRing.ReadIdx);

				spin_unlock_irqrestore(&
						       (EAFInfo.
							SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]),
						       flags);

				mutex_unlock(&gEafDequeMutex);

				if (eaf_EafReq.m_pEafConfig == NULL) {
					LOG_ERR("NULL pointer");
					Ret = -EFAULT;
					goto EXIT;
				}

				if (copy_to_user((void *)eaf_EafReq.m_pEafConfig,
				     &g_EafDequeReq_Struct.EafFrameConfig[0], dequeNum * sizeof(EAF_Config)) != 0) {
					LOG_ERR("EAF_EAF_DEQUE_REQ copy_to_user frameconfig failed\n");
					Ret = -EFAULT;
				}
				if (copy_to_user((void *)Param, &eaf_EafReq, sizeof(EAF_Request)) != 0) {
					LOG_ERR("EAF_EAF_DEQUE_REQ copy_to_user failed\n");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("EAF_CMD_EAF_DEQUE_REQ copy_from_user failed\n");
				Ret = -EFAULT;
			}

			break;
		}
	default:
		{
			LOG_ERR("Unknown Cmd(%d)", Cmd);
			LOG_ERR("Fail, Cmd(%d), Dir(%d), Type(%d), Nr(%d),Size(%d)\n", Cmd,
				_IOC_DIR(Cmd), _IOC_TYPE(Cmd), _IOC_NR(Cmd), _IOC_SIZE(Cmd));
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
static int compat_get_EAF_read_register_data(compat_EAF_REG_IO_STRUCT __user *data32,
					     EAF_REG_IO_STRUCT __user *data)
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

static int compat_put_EAF_read_register_data(compat_EAF_REG_IO_STRUCT __user *data32,
					     EAF_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr; */
	int err = 0;
	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->pData); */
	/* err |= put_user(uptr, &data32->pData); */
	err |= get_user(count, &data->Count);
	err |= put_user(count, &data32->Count);
	return err;
}

static int compat_get_EAF_enque_req_data(compat_EAF_Request __user *data32,
					      EAF_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pEafConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pEafConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_EAF_enque_req_data(compat_EAF_Request __user *data32,
					      EAF_Request __user *data)
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


static int compat_get_EAF_deque_req_data(compat_EAF_Request __user *data32,
					      EAF_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pEafConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pEafConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_EAF_deque_req_data(compat_EAF_Request __user *data32,
					      EAF_Request __user *data)
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

static long EAF_ioctl_compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;


	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		LOG_ERR("no f_op !!!\n");
		return -ENOTTY;
	}
	switch (cmd) {
	case COMPAT_EAF_READ_REGISTER:
		{
			compat_EAF_REG_IO_STRUCT __user *data32;
			EAF_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_EAF_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_get_EAF_read_register_data error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, EAF_READ_REGISTER,
						       (unsigned long)data);
			err = compat_put_EAF_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_put_EAF_read_register_data error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_EAF_WRITE_REGISTER:
		{
			compat_EAF_REG_IO_STRUCT __user *data32;
			EAF_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_EAF_read_register_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_EAF_WRITE_REGISTER error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, EAF_WRITE_REGISTER,
						       (unsigned long)data);
			return ret;
		}
	case COMPAT_EAF_ENQUE_REQ:
		{
			compat_EAF_Request __user *data32;
			EAF_Request __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_EAF_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_EAF_ENQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, EAF_ENQUE_REQ,
						       (unsigned long)data);
			err = compat_put_EAF_enque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_EAF_ENQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_EAF_DEQUE_REQ:
		{
			compat_EAF_Request __user *data32;
			EAF_Request __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_EAF_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_EAF_DEQUE_REQ error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, EAF_DEQUE_REQ,
						       (unsigned long)data);
			err = compat_put_EAF_deque_req_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_EAF_DEQUE_REQ error!!!\n");
				return err;
			}
			return ret;
		}
	case EAF_WAIT_IRQ:
	case EAF_CLEAR_IRQ:	/* structure (no pointer) */
	case EAF_ENQNUE_NUM:
	case EAF_ENQUE:
	case EAF_DEQUE_NUM:
	case EAF_DEQUE:
	case EAF_RESET:
	case EAF_DUMP_REG:
	case EAF_DUMP_ISR_LOG:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return EAF_ioctl(filep, cmd, arg); */
	}
}

#endif

/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_open(struct inode *pInode, struct file *pFile)
{
	MINT32 Ret = 0;
	MUINT32 i, j;
	/*int q = 0, p = 0; */
	EAF_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("- E. UserCount: %d.", EAFInfo.UserCount);
	LOG_INF("EAF_open\n");


	/*  */
	spin_lock(&(EAFInfo.SpinLockEAFRef));

	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(EAF_USER_INFO_STRUCT), GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (EAF_USER_INFO_STRUCT *) pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
	if (EAFInfo.UserCount > 0) {
		EAFInfo.UserCount++;
		spin_unlock(&(EAFInfo.SpinLockEAFRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			EAFInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else {
		EAFInfo.UserCount++;
		spin_unlock(&(EAFInfo.SpinLockEAFRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), first user",
			EAFInfo.UserCount, current->comm, current->pid, current->tgid);
	}

	/* do wait queue head init when re-enter in camera */
	/*  */
	for (i = 0; i < _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_; i++) {
		/* EAF */
		g_EAF_ReqRing.EAFReq_Struct[i].processID = 0x0;
		g_EAF_ReqRing.EAFReq_Struct[i].callerID = 0x0;
		g_EAF_ReqRing.EAFReq_Struct[i].enqueReqNum = 0x0;
		/* g_EAF_ReqRing.EAFReq_Struct[i].enqueIdx = 0x0; */
		g_EAF_ReqRing.EAFReq_Struct[i].State = EAF_REQUEST_STATE_EMPTY;
		g_EAF_ReqRing.EAFReq_Struct[i].FrameWRIdx = 0x0;
		g_EAF_ReqRing.EAFReq_Struct[i].FrameRDIdx = 0x0;
		for (j = 0; j < _SUPPORT_MAX_EAF_FRAME_REQUEST_; j++)
			g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j] = EAF_FRAME_STATUS_EMPTY;

	}
	g_EAF_ReqRing.WriteIdx = 0x0;
	g_EAF_ReqRing.ReadIdx = 0x0;
	g_EAF_ReqRing.HWProcessIdx = 0x0;

	/* Enable clock */
	EAF_EnableClock(MTRUE);
	LOG_DBG("EAF open g_u4EnableClockCount: %d", g_u4EnableClockCount);
	/*  */

	for (i = 0; i < EAF_IRQ_TYPE_AMOUNT; i++)
		EAFInfo.IrqInfo.Status[i] = 0;

	for (i = 0; i < _SUPPORT_MAX_EAF_FRAME_REQUEST_; i++)
		EAFInfo.ProcessID[i] = 0;

	EAFInfo.WriteReqIdx = 0;
	EAFInfo.ReadReqIdx = 0;
	EAFInfo.IrqInfo.EafIrqCnt = 0;


/* //QQ#ifdef KERNEL_LOG */
	/* In EP, Add EAF_DBG_WRITE_REG for debug. Should remove it after EP */
	EAFInfo.DebugMask = (EAF_DBG_INT | EAF_DBG_DBGLOG | EAF_DBG_WRITE_REG);
/*#endif*/
	/*  */
EXIT:

	LOG_DBG("- X. Ret: %d. UserCount: %d.", Ret, EAFInfo.UserCount);
	return Ret;

}

/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_release(struct inode *pInode, struct file *pFile)
{
	EAF_USER_INFO_STRUCT *pUserInfo;
	/*MUINT32 Reg; */

	LOG_DBG("- E. UserCount: %d.", EAFInfo.UserCount);

	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo = (EAF_USER_INFO_STRUCT *) pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	/*  */
	spin_lock(&(EAFInfo.SpinLockEAFRef));
	EAFInfo.UserCount--;

	if (EAFInfo.UserCount > 0) {
		spin_unlock(&(EAFInfo.SpinLockEAFRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			EAFInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else
		spin_unlock(&(EAFInfo.SpinLockEAFRef));
	/*  */
	LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), last user",
		EAFInfo.UserCount, current->comm, current->pid, current->tgid);


	/* Disable clock. */
	EAF_EnableClock(MFALSE);
	LOG_DBG("EAF release g_u4EnableClockCount: %d", g_u4EnableClockCount);

	/*  */
EXIT:


	LOG_DBG("- X. UserCount: %d.", EAFInfo.UserCount);
	return 0;
}


/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	long length = 0;
	MUINT32 pfn = 0x0;

	length = pVma->vm_end - pVma->vm_start;
	/*  */
	pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
	pfn = pVma->vm_pgoff << PAGE_SHIFT;

	LOG_INF("EAF_mmap: pVma->vm_pgoff(0x%lx)", pVma->vm_pgoff);
	LOG_INF("EAF_mmap: pfn(0x%x),phy(0x%lx)", pfn, pVma->vm_pgoff << PAGE_SHIFT);
	LOG_INF("pVmapVma->vm_start(0x%lx)", pVma->vm_start);
	LOG_INF("pVma->vm_end(0x%lx),length(0x%lx)", pVma->vm_end, length);


	switch (pfn) {
	case EAF_BASE_HW:
		if (length > EAF_REG_RANGE) {
			LOG_ERR("mmap range error :module:0x%x length(0x%lx),EAF_REG_RANGE(0x%x)!",
				pfn, length, EAF_REG_RANGE);
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

static dev_t EAFDevNo;
static struct cdev *pEAFCharDrv;
static struct class *pEAFClass;

static const struct file_operations EAFFileOper = {
	.owner = THIS_MODULE,
	.open = EAF_open,
	.release = EAF_release,
	/* .flush   = mt_EAF_flush, */
	.mmap = EAF_mmap,
	.unlocked_ioctl = EAF_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = EAF_ioctl_compat,
#endif
};

/*******************************************************************************
*
********************************************************************************/
static inline void EAF_UnregCharDev(void)
{
	LOG_DBG("- E.");
	/*  */
	/* Release char driver */
	if (pEAFCharDrv != NULL) {
		cdev_del(pEAFCharDrv);
		pEAFCharDrv = NULL;
	}
	/*  */
	unregister_chrdev_region(EAFDevNo, 1);
}

/*******************************************************************************
*
********************************************************************************/
static inline MINT32 EAF_RegCharDev(void)
{
	MINT32 Ret = 0;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = alloc_chrdev_region(&EAFDevNo, 0, 1, EAF_DEV_NAME);
	if (Ret < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d", Ret);
		return Ret;
	}
	/* Allocate driver */
	pEAFCharDrv = cdev_alloc();
	if (pEAFCharDrv == NULL) {
		LOG_ERR("cdev_alloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/* Attatch file operation. */
	cdev_init(pEAFCharDrv, &EAFFileOper);
	/*  */
	pEAFCharDrv->owner = THIS_MODULE;
	/* Add to system */
	Ret = cdev_add(pEAFCharDrv, EAFDevNo, 1);
	if (Ret < 0) {
		LOG_ERR("Attatch file operation failed, %d", Ret);
		goto EXIT;
	}
	/*  */
EXIT:
	if (Ret < 0)
		EAF_UnregCharDev();

	/*  */

	LOG_DBG("- X.");
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_probe(struct platform_device *pDev)
{
	MINT32 Ret = 0;
	/*struct resource *pRes = NULL; */
	MINT32 i = 0;
	MUINT8 n;
	MUINT32 irq_info[3];	/* Record interrupts info from device tree */
	struct device *dev = NULL;
	struct EAF_device *_eafdev = NULL;

#ifdef CONFIG_OF
	struct EAF_device *EAF_dev;
#endif

	/* Check platform_device parameters */
#ifdef CONFIG_OF

	if (pDev == NULL) {
		dev_err(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}

	nr_EAF_devs += 1;
	_eafdev = krealloc(EAF_devs, sizeof(struct EAF_device) * nr_EAF_devs, GFP_KERNEL);
	if (!_eafdev) {
		dev_err(&pDev->dev, "Unable to allocate EAF_devs\n");
		return -ENOMEM;
	}
	EAF_devs = _eafdev;

	EAF_dev = &(EAF_devs[nr_EAF_devs - 1]);
	EAF_dev->dev = &pDev->dev;

	/* iomap registers */
	EAF_dev->regs = of_iomap(pDev->dev.of_node, 0);
	/* gISPSYS_Reg[nr_EAF_devs - 1] = EAF_dev->regs; */

	if (!EAF_dev->regs) {
		dev_err(&pDev->dev,
			"Unable to ioremap registers, of_iomap fail, nr_EAF_devs=%d, devnode(%s).\n",
			nr_EAF_devs, pDev->dev.of_node->name);
		return -ENOMEM;
	}

	LOG_INF("nr_EAF_devs=%d, devnode(%s), map_addr=0x%lx\n", nr_EAF_devs,
		pDev->dev.of_node->name, (unsigned long)EAF_dev->regs);

	/* get IRQ ID and request IRQ */
	EAF_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);

	if (EAF_dev->irq > 0) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array
		    (pDev->dev.of_node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
			dev_err(&pDev->dev, "get irq flags from DTS fail!!\n");
			return -ENODEV;
		}

		for (i = 0; i < EAF_IRQ_TYPE_AMOUNT; i++) {
			if (strcmp(pDev->dev.of_node->name, EAF_IRQ_CB_TBL[i].device_name) == 0) {
				Ret =
				    request_irq(EAF_dev->irq,
						(irq_handler_t) EAF_IRQ_CB_TBL[i].isr_fp,
						irq_info[2],
						(const char *)EAF_IRQ_CB_TBL[i].device_name, NULL);
				if (Ret) {
					dev_err(&pDev->dev,
						"Unable to request IRQ, request_irq fail, nr_EAF_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
						nr_EAF_devs, pDev->dev.of_node->name, EAF_dev->irq,
						EAF_IRQ_CB_TBL[i].device_name);
					return Ret;
				}

				LOG_INF("nr_EAF_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
					nr_EAF_devs, pDev->dev.of_node->name, EAF_dev->irq,
					EAF_IRQ_CB_TBL[i].device_name);
				break;
			}
		}

		if (i > EAF_IRQ_TYPE_AMOUNT) {
			LOG_INF("No corresponding ISR!!: nr_EAF_devs=%d, devnode(%s), irq=%d\n",
				nr_EAF_devs, pDev->dev.of_node->name, EAF_dev->irq);
		}


	} else {
		LOG_INF("No IRQ!!: nr_EAF_devs=%d, devnode(%s), irq=%d\n", nr_EAF_devs,
			pDev->dev.of_node->name, EAF_dev->irq);
	}


#endif

	/* Only register char driver in the 1st time */
	if (nr_EAF_devs == 1) {

		/* Register char driver */
		Ret = EAF_RegCharDev();
		if (Ret) {
			dev_err(&pDev->dev, "register char failed");
			return Ret;
		}

#ifndef __EAF_EP_NO_CLKMGR__
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
		    /*CCF: Grab clock pointer (struct clk*) */
		eaf_clk.CG_SCP_SYS_MM0 = devm_clk_get(&pDev->dev, "EAF_SCP_SYS_MM0");
		eaf_clk.CG_MM_SMI_COMMON = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG2_B11");
		eaf_clk.CG_MM_SMI_COMMON_2X = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG2_B12");
		eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B12");
		eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B13");
		eaf_clk.CG_MM_SMI_COMMON_UPSZ0 = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B14");
		eaf_clk.CG_MM_SMI_COMMON_UPSZ1 = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B15");
		eaf_clk.CG_MM_SMI_COMMON_FIFO0 = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B16");
		eaf_clk.CG_MM_SMI_COMMON_FIFO1 = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B17");
		eaf_clk.CG_MM_LARB5 = devm_clk_get(&pDev->dev, "EAF_CLK_MM_CG1_B10");
		eaf_clk.CG_SCP_SYS_ISP = devm_clk_get(&pDev->dev, "EAF_SCP_SYS_ISP");
		eaf_clk.CG_IMGSYS_LARB = devm_clk_get(&pDev->dev, "EAF_CLK_IMG_LARB");
		eaf_clk.CG_IMGSYS_EAF = devm_clk_get(&pDev->dev, "EAF_CLK_IMG_EAF");

		if (IS_ERR(eaf_clk.CG_SCP_SYS_MM0)) {
			LOG_ERR("cannot get CG_SCP_SYS_MM0 clock\n");
			return PTR_ERR(eaf_clk.CG_SCP_SYS_MM0);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_2X clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_2X);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_GALS_M0_2X clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_GALS_M1_2X clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_UPSZ0)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_UPSZ0 clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_UPSZ0);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_UPSZ1)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_UPSZ1 clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_UPSZ1);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_FIFO0)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_FIFO0 clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_FIFO0);
		}
		if (IS_ERR(eaf_clk.CG_MM_SMI_COMMON_FIFO1)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_FIFO1 clock\n");
			return PTR_ERR(eaf_clk.CG_MM_SMI_COMMON_FIFO1);
		}
		if (IS_ERR(eaf_clk.CG_MM_LARB5)) {
			LOG_ERR("cannot get CG_MM_LARB5 clock\n");
			return PTR_ERR(eaf_clk.CG_MM_LARB5);
		}
		if (IS_ERR(eaf_clk.CG_SCP_SYS_ISP)) {
			LOG_ERR("cannot get CG_SCP_SYS_ISP clock\n");
			return PTR_ERR(eaf_clk.CG_SCP_SYS_ISP);
		}
		if (IS_ERR(eaf_clk.CG_IMGSYS_LARB)) {
			LOG_ERR("cannot get CG_IMGSYS_LARB clock\n");
			return PTR_ERR(eaf_clk.CG_IMGSYS_LARB);
		}
		if (IS_ERR(eaf_clk.CG_IMGSYS_EAF)) {
			LOG_ERR("cannot get CG_IMGSYS_EAF clock\n");
			return PTR_ERR(eaf_clk.CG_IMGSYS_EAF);
		}
#endif				/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
#endif

		/* Create class register */
		pEAFClass = class_create(THIS_MODULE, "EAFdrv");
		if (IS_ERR(pEAFClass)) {
			Ret = PTR_ERR(pEAFClass);
			LOG_ERR("Unable to create class, err = %d", Ret);
			goto EXIT;
		}

		dev = device_create(pEAFClass, NULL, EAFDevNo, NULL, EAF_DEV_NAME);

		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_err(&pDev->dev, "Failed to create device: /dev/%s, err = %d",
				EAF_DEV_NAME, Ret);
			goto EXIT;
		}

		/* Init spinlocks */
		spin_lock_init(&(EAFInfo.SpinLockEAFRef));
		spin_lock_init(&(EAFInfo.SpinLockEAF));
		for (n = 0; n < EAF_IRQ_TYPE_AMOUNT; n++)
			spin_lock_init(&(EAFInfo.SpinLockIrq[n]));

		/*  */
		init_waitqueue_head(&EAFInfo.WaitQueueHead);
		INIT_WORK(&EAFInfo.ScheduleEafWork, EAF_ScheduleWork);

		wake_lock_init(&EAF_wake_lock, WAKE_LOCK_SUSPEND, "EAF_lock_wakelock");

		for (i = 0; i < EAF_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(EAF_tasklet[i].pEAF_tkt, EAF_tasklet[i].tkt_cb, 0);

		/* Init EAFInfo */
		spin_lock(&(EAFInfo.SpinLockEAFRef));
		EAFInfo.UserCount = 0;
		spin_unlock(&(EAFInfo.SpinLockEAFRef));
		/*  */
		EAFInfo.IrqInfo.Mask[EAF_IRQ_TYPE_INT_EAF_ST] = INT_ST_MASK_EAF;

	}

EXIT:
	if (Ret < 0)
		EAF_UnregCharDev();


	LOG_INF("- X. EAF driver probe.");

	return Ret;
}

/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static MINT32 EAF_remove(struct platform_device *pDev)
{
	/*struct resource *pRes; */
	MINT32 IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	/* unregister char driver. */
	EAF_UnregCharDev();

	/* Release IRQ */
	disable_irq(EAFInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* kill tasklet */
	for (i = 0; i < EAF_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(EAF_tasklet[i].pEAF_tkt);
#if 0
	/* free all registered irq(child nodes) */
	EAF_UnRegister_AllregIrq();
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
	device_destroy(pEAFClass, EAFDevNo);
	/*  */
	class_destroy(pEAFClass);
	pEAFClass = NULL;
	/*  */
	return 0;
}

/*******************************************************************************
*
********************************************************************************/

static MINT32 EAF_suspend(struct platform_device *pDev, pm_message_t Mesg)
{
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 EAF_resume(struct platform_device *pDev)
{
	return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int EAF_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return EAF_suspend(pdev, PMSG_SUSPEND);
}

int EAF_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return EAF_resume(pdev);
}
#ifndef CONFIG_OF
/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
#endif
int EAF_pm_restore_noirq(struct device *device)
{
	pr_debug("calling %s()\n", __func__);
#ifndef CONFIG_OF
	mt_irq_set_sens(EAF_IRQ_BIT_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(EAF_IRQ_BIT_ID, MT_POLARITY_LOW);
#endif
	return 0;

}

/*---------------------------------------------------------------------------*/
#else				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#define EAF_pm_suspend NULL
#define EAF_pm_resume  NULL
#define EAF_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_OF
/*
 * Note!!! The order and member of .compatible must be the same with that in
 *  "EAF_DEV_NODE_ENUM" in camera_EAF.h
 */
static const struct of_device_id EAF_of_ids[] = {
	{.compatible = "mediatek,eaf", },
	{}
};
#endif

const struct dev_pm_ops EAF_pm_ops = {
	.suspend = EAF_pm_suspend,
	.resume = EAF_pm_resume,
	.freeze = EAF_pm_suspend,
	.thaw = EAF_pm_resume,
	.poweroff = EAF_pm_suspend,
	.restore = EAF_pm_resume,
	.restore_noirq = EAF_pm_restore_noirq,
};


/*******************************************************************************
*
********************************************************************************/
static struct platform_driver EAFDriver = {
	.probe = EAF_probe,
	.remove = EAF_remove,
	.suspend = EAF_suspend,
	.resume = EAF_resume,
	.driver = {
		   .name = EAF_DEV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = EAF_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &EAF_pm_ops,
#endif
		   }
};


static int EAF_dump_read(struct seq_file *m, void *v)
{
	int i, j;

	seq_puts(m, "\n============ EAF dump register============\n");
	seq_puts(m, "EAF Config Info\n");

	for (i = 0; i <= 0x258; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	for (i = 0x300; i <= 0x328; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	for (i = 0x380; i <= 0x3A0; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	for (i = 0x800; i <= 0xA38; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}
	seq_puts(m, "EAF Debug Info\n");
	for (i = 0x4A8; i < 0x4B8; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	seq_puts(m, "\n");
	seq_printf(m, "EAF Clock Count:%d\n", g_u4EnableClockCount);


	seq_printf(m, "EAF:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		   g_EAF_ReqRing.HWProcessIdx, g_EAF_ReqRing.WriteIdx, g_EAF_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_EAF_REQUEST_RING_SIZE_; i++) {
		seq_printf(m, "EAF:State:%d, processID:0x%08X, callerID:0x%08X, enqueReqNum:%d, FrameWRIdx:%d, FrameRDIdx:%d\n",
			   g_EAF_ReqRing.EAFReq_Struct[i].State,
			   g_EAF_ReqRing.EAFReq_Struct[i].processID,
			   g_EAF_ReqRing.EAFReq_Struct[i].callerID,
			   g_EAF_ReqRing.EAFReq_Struct[i].enqueReqNum,
			   g_EAF_ReqRing.EAFReq_Struct[i].FrameWRIdx,
			   g_EAF_ReqRing.EAFReq_Struct[i].FrameRDIdx);

		for (j = 0; j < _SUPPORT_MAX_EAF_FRAME_REQUEST_;) {
			seq_printf(m,
				   "EAF:FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d, FrameStatus[%d]:%d\n",
				   j, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j],
				   j + 1, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j + 1],
				   j + 2, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j + 2],
				   j + 3, g_EAF_ReqRing.EAFReq_Struct[i].EafFrameStatus[j + 3]);
			j = j + 4;
		}
	}

	seq_puts(m, "\n============ EAF dump debug ============\n");

	return 0;
}

static int proc_EAF_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, EAF_dump_read, NULL);
}

static const struct file_operations EAF_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_EAF_dump_open,
	.read = seq_read,
};


static int EAF_reg_read(struct seq_file *m, void *v)
{
	unsigned int i;

	seq_puts(m, "======== read eaf register ========\n");

	for (i = 0; i <= 0x258; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	for (i = 0x300; i <= 0x328; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	for (i = 0x380; i <= 0x3A0; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}

	for (i = 0x800; i <= 0xA38; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(EAF_BASE_HW + i),
			   (unsigned int)EAF_RD32(ISP_EAF_BASE + i));
	}
	seq_puts(m, "======== read eaf register done ========\n");

	return 0;
}

/*static int EAF_reg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)*/

static ssize_t EAF_reg_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
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
				LOG_INF("EAF Write Addr Error!!:%s", addrSzBuf);
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
				LOG_INF("EAF Write Value Error!!:%s\n", valSzBuf);
			}
		}

		if ((addr >= EAF_BASE_HW) && (addr <= EAF_DBG_CTL_HW)) {
			LOG_INF("Write Request - addr:0x%x, value:0x%x\n", addr, val);
			EAF_WR32((ISP_EAF_BASE + (addr - EAF_BASE_HW)), val);
		} else {
			LOG_INF
			    ("Write-Address Range exceeds the size of hw EAF!! addr:0x%x, value:0x%x\n",
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
				LOG_INF("EAF Read Addr Error!!:%s", addrSzBuf);
			}
		}

		if ((addr >= EAF_BASE_HW) && (addr <= EAF_DBG_CTL_HW)) {
			val = EAF_RD32((ISP_EAF_BASE + (addr - EAF_BASE_HW)));
			LOG_INF("Read Request - addr:0x%x,value:0x%x\n", addr, val);
		} else {
			LOG_INF
			    ("Read-Address Range exceeds the size of hw EAF!! addr:0x%x, value:0x%x\n",
			     addr, val);
		}

	}


	return count;
}

static int proc_EAF_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, EAF_reg_read, NULL);
}

static const struct file_operations EAF_reg_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_EAF_reg_open,
	.read = seq_read,
	.write = EAF_reg_write,
};


/*******************************************************************************
*
********************************************************************************/
int32_t EAF_ClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("EAF_ClockOnCallback"); */
	/* LOG_DBG("+CmdqEn:%d", g_u4EnableClockCount); */
	/* EAF_EnableClock(MTRUE); */

	return 0;
}

int32_t EAF_DumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("GEPF_DumpCallback");

	EAF_DumpReg();

	return 0;
}

int32_t EAF_ResetCallback(uint64_t engineFlag)
{
	LOG_DBG("GEPF_ResetCallback");
	EAF_Reset();

	return 0;
}

int32_t EAF_ClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("GEPF_ClockOffCallback"); */
	/* GEPF_EnableClock(MFALSE); */
	/* LOG_DBG("-CmdqEn:%d", g_u4EnableClockCount); */
	return 0;
}


static MINT32 __init EAF_Init(void)
{
	MINT32 Ret = 0, j;
	void *tmp;
	/* FIX-ME: linux-3.10 procfs API changed */
	/* use proc_create */
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *isp_EAF_dir;


	int i;
	/*  */
	LOG_INF("- E.");

	LOG_INF("EAF_Init\n");

	/*  */
	Ret = platform_driver_register(&EAFDriver);
	if (Ret < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}
#if 0
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,eaf");
	if (!node) {
		LOG_ERR("find mediatek,EAF node failed!!!\n");
		return -ENODEV;
	}
	ISP_EAF_BASE = of_iomap(node, 0);
	if (!ISP_EAF_BASE) {
		LOG_ERR("unable to map ISP_EAF_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_EAF_BASE: %lx\n", ISP_EAF_BASE);
#endif

	isp_EAF_dir = proc_mkdir("eaf", NULL);
	if (!isp_EAF_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/EAF\n", __func__);
		return 0;
	}

	/* proc_entry = proc_create("pll_test", S_IRUGO | S_IWUSR, isp_EAF_dir, &pll_test_proc_fops); */

	proc_entry = proc_create("EAF_dump", S_IRUGO, isp_EAF_dir, &EAF_dump_proc_fops);

	proc_entry = proc_create("EAF_reg", S_IRUGO | S_IWUSR, isp_EAF_dir, &EAF_reg_proc_fops);


	/* isr log */
	if (PAGE_SIZE <
	    ((EAF_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN * ((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM)) {
		i = 0;
		while (i <
		       ((EAF_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
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
		for (j = 0; j < EAF_IRQ_TYPE_AMOUNT; j++) {
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


	/* Cmdq */
	/* Register EAF callback */
#ifdef EAF_USE_GCE

	LOG_DBG("register eaf callback for CMDQ");
	cmdqCoreRegisterCB(CMDQ_GROUP_EAF,
			   EAF_ClockOnCallback,
			   EAF_DumpCallback, EAF_ResetCallback, EAF_ClockOffCallback);
#endif
	LOG_DBG("- X. Ret: %d.", Ret);
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static void __exit EAF_Exit(void)
{
	/*int i; */

	LOG_INF("- E.");
	/*  */
	platform_driver_unregister(&EAFDriver);
	/*  */
#ifdef EAF_USE_GCE

	/* Cmdq */
	/* Unregister EAF callback */
	cmdqCoreRegisterCB(CMDQ_GROUP_EAF, NULL, NULL, NULL, NULL);
#endif
	kfree(pLog_kmalloc);

	/*  */
}


/*******************************************************************************
*
********************************************************************************/
static void EAF_ScheduleWork(struct work_struct *data)
{
	if (EAF_DBG_DBGLOG & EAFInfo.DebugMask)
		LOG_DBG("- E.");
}



static irqreturn_t ISP_Irq_EAF(signed int Irq, void *DeviceId)
{
	unsigned int EAFStatus;
	bool bResulst = MFALSE;
	pid_t ProcessID;

	LOG_INF("ISP_Irq_EAF\n");

	EAFStatus = EAF_RD32(EAF_INT_STATUS_REG);	/* EAF Status */
	LOG_INF("EAF INTERRUPT Handler reads EAF_INT_STATUS(%d)", EAFStatus);

	spin_lock(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]));

	if (EAF_INT_ST == (EAF_INT_ST & EAFStatus)) {
		/* Update the frame status. */
#ifdef __EAF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("EAF_irq");
#endif

		EAF_WR32(EAF_INT_STATUS_REG, 1);/* clear IRQ status */

		bResulst = UpdateEAF(&ProcessID);
		/* ConfigEAF(); */
		if (bResulst == MTRUE) {
			schedule_work(&EAFInfo.ScheduleEafWork);

			EAFInfo.IrqInfo.Status[EAF_IRQ_TYPE_INT_EAF_ST] |= EAF_INT_ST;
			EAFInfo.IrqInfo.ProcessID[EAF_PROCESS_ID_EAF] = ProcessID;
			EAFInfo.IrqInfo.EafIrqCnt++;
			EAFInfo.ProcessID[EAFInfo.WriteReqIdx] = ProcessID;
			EAFInfo.WriteReqIdx =
			    (EAFInfo.WriteReqIdx + 1) % _SUPPORT_MAX_EAF_FRAME_REQUEST_;
#ifdef EAF_MULTIPROCESS_TIMEING_ISSUE
			/* check the write value is equal to read value ? */
			/* actually, it doesn't happen!! */
			if (EAFInfo.WriteReqIdx == EAFInfo.ReadReqIdx) {
				IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_ERR,
					       "ISP_Irq_EAF Err!!, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
					       EAFInfo.WriteReqIdx, EAFInfo.ReadReqIdx);
			}
#endif
		}
#ifdef __EAF_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}
	spin_unlock(&(EAFInfo.SpinLockIrq[EAF_IRQ_TYPE_INT_EAF_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&EAFInfo.WaitQueueHead);

	/* dump log, use tasklet */
	IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_INF,
		       "ISP_Irq_EAF:%d, reg 0x%x : 0x%x, bResulst:%d, EAFHWSta:0x%x, EafIrqCnt:0x%x, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n",
		       Irq, EAF_INT_STATUS_HW, EAFStatus, bResulst, EAFStatus,
		       EAFInfo.IrqInfo.EafIrqCnt,
		       EAFInfo.WriteReqIdx, EAFInfo.ReadReqIdx);
	/* IRQ_LOG_KEEPER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_INF, "EAFHWSta:0x%x, EAFHWSta:0x%x,
	*  DpeDveSta0:0x%x\n", DveStatus, EAFStatus, DpeDveSta0);
	*/

	if (EAFStatus & EAF_INT_ST)
		tasklet_schedule(EAF_tasklet[EAF_IRQ_TYPE_INT_EAF_ST].pEAF_tkt);

	return IRQ_HANDLED;
}

static void ISP_TaskletFunc_EAF(unsigned long data)
{
	IRQ_LOG_PRINTER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(EAF_IRQ_TYPE_INT_EAF_ST, m_CurrentPPB, _LOG_ERR);

}


/*******************************************************************************
*
********************************************************************************/
module_init(EAF_Init);
module_exit(EAF_Exit);
MODULE_DESCRIPTION("Camera EAF driver");
MODULE_AUTHOR("MM3_SW5");
MODULE_LICENSE("GPL");
