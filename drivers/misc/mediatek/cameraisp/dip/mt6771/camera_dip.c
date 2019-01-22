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
 * camera_dip.c - MT6799 Linux DIP Device Driver
 *
 * DESCRIPTION:
 *     This file provid the other drivers DIP relative functions
 *
 ******************************************************************************/
/* MET: define to enable MET*/
#define DIP_MET_READY

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h> /* proc file use */
#include <linux/slab.h>
#include <linux/spinlock.h>
/* #include <linux/io.h> */
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/seq_file.h>

/*#include <mach/irqs.h>*/
/*#include <mach/mt_clkmgr.h>*/     /* For clock mgr APIS. enable_clock()/disable_clock(). */
#include <mt-plat/sync_write.h> /* For mt65xx_reg_sync_writel(). */
/* #include <mach/mt_spm_idle.h> */   /* For spm_enable_sodi()/spm_disable_sodi(). */

#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#ifdef COFNIG_MTK_IOMMU
#include <mtk_iommu.h>
#else
#include <m4u.h>
#endif

#define EP_CODE_MARK_CMDQ
#ifndef EP_CODE_MARK_CMDQ
#include <cmdq_core.h>
#endif

#include <smi_public.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

/*for SMI BW debug log*/
#include"../../../smi/smi_debug.h"

/*for kernel log count*/
#define _K_LOG_ADJUST (1)

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/*  */
/* #include "smi_common.h" */

#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
#else
#include <linux/wakelock.h>
#endif

/*for kernel log reduction*/
#include <linux/printk.h>


#ifdef CONFIG_OF
#include <linux/of_platform.h>  /* for device tree */
#include <linux/of_irq.h>       /* for device tree */
#include <linux/of_address.h>   /* for device tree */
#endif

#if defined(DIP_MET_READY)
/*MET:met mmsys profile*/
#include <mt-plat/met_drv.h>
#endif

/*#define MMDVFS_PM_QOS_READY*/
#ifdef MMDVFS_PM_QOS_READY
#include "mmdvfs_mgr.h" /* wait for mmdvfs ready */
/* Use this qos request to control camera dynamic frequency change */
struct mmdvfs_pm_qos_request dip_qos;
#endif

#define CAMSV_DBG
#ifdef CAMSV_DBG
#define CAM_TAG "CAM:"
#define CAMSV_TAG "SV1:"
#define CAMSV2_TAG "SV2:"
#else
#define CAMSV_TAG ""
#define CAMSV2_TAG ""
#define CAM_TAG ""
#endif

#include "camera_dip.h"

/*  */
#ifndef MTRUE
#define MTRUE               1
#endif
#ifndef MFALSE
#define MFALSE              0
#endif

#define DIP_DEV_NAME                "camera-dip"
#define DUMMY_INT                   (0)   /*for early if load dont need to use camera*/

/* #define EP_NO_CLKMGR *//* Clkmgr is not ready in early porting, en/disable clock  by hardcode */

#define DIP_BOTTOMHALF_WORKQ		(1)

#if (DIP_BOTTOMHALF_WORKQ == 1)
#include <linux/workqueue.h>
#endif

/* ---------------------------------------------------------------------------- */

#define MyTag "[DIP]"
#define IRQTag "KEEPER"

#define LOG_VRB(format, args...)    pr_debug(MyTag "[%s] " format, __func__, ##args)

#define DIP_DEBUG
#ifdef DIP_DEBUG
#define LOG_DBG(format, args...)    pr_info(MyTag "[%s] " format, __func__, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...)    pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_NOTICE(format, args...) pr_notice(MyTag "[%s] " format, __func__, ##args)
#define LOG_WRN(format, args...)    pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...)    pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_AST(format, args...)    pr_debug(MyTag "[%s] " format, __func__, ##args)

/*******************************************************************************
*
********************************************************************************/
/* #define DIP_WR32(addr, data)    iowrite32(data, addr) // For other projects. */
#define DIP_WR32(addr, data)    mt_reg_sync_writel(data, addr)  /* For 89     Only.*/   /* NEED_TUNING_BY_PROJECT */
#define DIP_RD32(addr)                  ioread32((void *)addr)
/*******************************************************************************
*
********************************************************************************/
/* dynamic log level */
#define DIP_DBG_INT                 (0x00000001)
#define DIP_DBG_READ_REG            (0x00000004)
#define DIP_DBG_WRITE_REG           (0x00000008)
#define DIP_DBG_CLK                 (0x00000010)
#define DIP_DBG_TASKLET             (0x00000020)
#define DIP_DBG_SCHEDULE_WORK       (0x00000040)
#define DIP_DBG_BUF_WRITE           (0x00000080)
#define DIP_DBG_BUF_CTRL            (0x00000100)
#define DIP_DBG_REF_CNT_CTRL        (0x00000200)
#define DIP_DBG_INT_2               (0x00000400)
#define DIP_DBG_INT_3               (0x00000800)
#define DIP_DBG_HW_DON              (0x00001000)
#define DIP_DBG_ION_CTRL            (0x00002000)
/*******************************************************************************
*
********************************************************************************/
#define DUMP_GCE_TPIPE  0

static irqreturn_t DIP_Irq_DIP_A(signed int  Irq, void *DeviceId);


typedef irqreturn_t (*IRQ_CB)(signed int, void *);

struct ISR_TABLE {
	IRQ_CB          isr_fp;
	unsigned int    int_number;
	char            device_name[16];
};

#ifndef CONFIG_OF
const struct ISR_TABLE DIP_IRQ_CB_TBL[DIP_IRQ_TYPE_AMOUNT] = {
	{NULL,              0,    "DIP_A"}
};

#else
/* int number is got from kernel api */

const struct ISR_TABLE DIP_IRQ_CB_TBL[DIP_IRQ_TYPE_AMOUNT] = {
	{DIP_Irq_DIP_A,     0,  "dip"}
};

/*
 * Note!!! The order and member of .compatible must be the same with that in
 *  "DIP_DEV_NODE_ENUM" in camera_dip.h
 */
static const struct of_device_id dip_of_ids[] = {
/*	{ .compatible = "mediatek,imgsys", },*/
	{ .compatible = "mediatek,dip1", }, /* Remider: Add this device node manually in .dtsi */
	{}
};

#endif
/* //////////////////////////////////////////////////////////////////////////////////////////// */
/*  */
typedef void (*tasklet_cb)(unsigned long);
struct Tasklet_table {
	tasklet_cb tkt_cb;
	struct tasklet_struct  *pIsp_tkt;
};

struct tasklet_struct DIP_tkt[DIP_IRQ_TYPE_AMOUNT];

static struct Tasklet_table dip_tasklet[DIP_IRQ_TYPE_AMOUNT] = {
	{NULL,                  &DIP_tkt[DIP_IRQ_TYPE_INT_DIP_A_ST]},
};

#if (DIP_BOTTOMHALF_WORKQ == 1)
struct IspWorkqueTable {
	enum DIP_IRQ_TYPE_ENUM	module;
	struct work_struct  dip_bh_work;
};

static void DIP_BH_Workqueue(struct work_struct *pWork);

static struct IspWorkqueTable dip_workque[DIP_IRQ_TYPE_AMOUNT] = {
	{DIP_IRQ_TYPE_INT_DIP_A_ST},
};
#endif


#ifdef CONFIG_OF

/* TODO: Remove, Jessy */
enum {
	DIP_BASE_ADDR = 0,
	DIP_INNER_BASE_ADDR,
	DIP_IMGSYS_CONFIG_BASE_ADDR,
	DIP_MIPI_ANA_BASE_ADDR,
	DIP_GPIO_BASE_ADDR,
	DIP_CAM_BASEADDR_NUM
} DIP_CAM_BASEADDR_ENUM;

#ifndef CONFIG_MTK_CLKMGR /*CCF*/
#include <linux/clk.h>
struct DIP_CLK_STRUCT {
	struct clk *DIP_IMG_LARB5;
	struct clk *DIP_IMG_LARB2;
	struct clk *DIP_IMG_DIP;
};
struct DIP_CLK_STRUCT dip_clk;
#endif

/* TODO: Remove, Jessy */
/*static unsigned long gDIPSYS_Irq[DIP_IRQ_TYPE_AMOUNT];*/
/*static unsigned long gDIPSYS_Reg[DIP_CAM_BASEADDR_NUM];*/

#ifdef CONFIG_OF
struct dip_device {
	void __iomem *regs;
	struct device *dev;
	int irq;
};

static struct dip_device *dip_devs;
static int nr_dip_devs;
#endif

/*#define AEE_DUMP_BY_USING_ION_MEMORY*/
#define AEE_DUMP_REDUCE_MEMORY
#ifdef AEE_DUMP_REDUCE_MEMORY
/* ion */

#ifdef AEE_DUMP_BY_USING_ION_MEMORY
#include <ion.h>
#include <mtk/ion_drv.h>
#include <mtk/mtk_ion.h>

struct dip_imem_memory {
	void *handle;
	int ion_fd;
	uint64_t va;
	uint32_t pa;
	uint32_t length;
};

static struct ion_client *dip_p2_ion_client;
static struct dip_imem_memory g_dip_p2_imem_buf;
#endif
static bool g_bIonBufferAllocated;
static unsigned int *g_pPhyDIPBuffer;
/* Kernel Warning */
static unsigned int *g_pKWTpipeBuffer;
static unsigned int *g_pKWCmdqBuffer;
static unsigned int *g_pKWVirDIPBuffer;
/* Navtive Exception */
static unsigned int *g_pTuningBuffer;
static unsigned int *g_pTpipeBuffer;
static unsigned int *g_pVirDIPBuffer;
static unsigned int *g_pCmdqBuffer;
#else
/* Kernel Warning */
static unsigned int g_KWTpipeBuffer[(MAX_DIP_TILE_TDR_HEX_NO >> 2)];
static unsigned int g_KWCmdqBuffer[(MAX_DIP_CMDQ_BUFFER_SIZE >> 2)];
static unsigned int g_KWVirDIPBuffer[(DIP_DIP_REG_SIZE >> 2)];
/* Navtive Exception */
static unsigned int g_PhyDIPBuffer[(DIP_DIP_REG_SIZE >> 2)];
static unsigned int g_TuningBuffer[(DIP_DIP_REG_SIZE >> 2)];
static unsigned int g_TpipeBuffer[(MAX_DIP_TILE_TDR_HEX_NO >> 2)];
static unsigned int g_VirDIPBuffer[(DIP_DIP_REG_SIZE >> 2)];
static unsigned int g_CmdqBuffer[(MAX_DIP_CMDQ_BUFFER_SIZE >> 2)];
#endif
static bool g_bUserBufIsReady = MFALSE;
static unsigned int DumpBufferField;
static bool g_bDumpPhyDIPBuf = MFALSE;
static unsigned int g_tdriaddr = 0xffffffff;
static unsigned int g_cmdqaddr = 0xffffffff;
static struct DIP_GET_DUMP_INFO_STRUCT g_dumpInfo = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static struct DIP_MEM_INFO_STRUCT g_TpipeBaseAddrInfo = {0x0, 0x0, NULL, 0x0};
static struct DIP_MEM_INFO_STRUCT g_CmdqBaseAddrInfo = {0x0, 0x0, NULL, 0x0};
static unsigned int m_CurrentPPB;

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source dip_wake_lock;
#else
struct wake_lock dip_wake_lock;
#endif
static int g_bWaitLock;
/*
* static void __iomem *g_dip_base_dase;
* static void __iomem *g_dip_inner_base_dase;
* static void __iomem *g_imgsys_config_base_dase;
*/

/* Get HW modules' base address from device nodes */
#define DIP_IMGSYS_CONFIG_BASE          (dip_devs[DIP_DIP_A_IDX].regs)
#define DIP_DIP_A_BASE                  (dip_devs[DIP_DIP_A_IDX].regs)

void __iomem *DIP_SENINF0_BASE;
void __iomem *DIP_SENINF1_BASE;
void __iomem *DIP_SENINF2_BASE;
void __iomem *DIP_SENINF3_BASE;

void __iomem *DIP_CLOCK_CELL_BASE;

void __iomem *DIP_MMSYS_CONFIG_BASE;

/* TODO: Remove start, Jessy */
#define DIP_ADDR                        (gDIPSYS_Reg[DIP_BASE_ADDR])
#define DIP_IMGSYS_BASE                 (gDIPSYS_Reg[DIP_IMGSYS_CONFIG_BASE_ADDR])
#define DIP_ADDR_CAMINF                 (gDIPSYS_Reg[DIP_IMGSYS_CONFIG_BASE_ADDR])

#define DIP_MIPI_ANA_ADDR               (gDIPSYS_Reg[DIP_MIPI_ANA_BASE_ADDR])
#define DIP_GPIO_ADDR                   (gDIPSYS_Reg[DIP_GPIO_BASE_ADDR])

#define DIP_IMGSYS_BASE_PHY             0x15000000

#else
#define DIP_ADDR                        (IMGSYS_BASE + 0x4000)
#define DIP_IMGSYS_BASE                 IMGSYS_BASE
#define DIP_ADDR_CAMINF                 IMGSYS_BASE
#define DIP_MIPI_ANA_ADDR               0x10217000
#define DIP_GPIO_ADDR                   GPIO_BASE

#endif
/* TODO: Remove end, Jessy */


#define DIP_REG_SW_CTL_RST_CAM_P1       (1)
#define DIP_REG_SW_CTL_RST_CAM_P2       (2)
#define DIP_REG_SW_CTL_RST_CAMSV        (3)
#define DIP_REG_SW_CTL_RST_CAMSV2       (4)

struct S_START_T {
	unsigned int sec;
	unsigned int usec;
};

/* QQ, remove later */
/* record remain node count(success/fail) excludes head when enque/deque control */
static unsigned int g_regScen = 0xa5a5a5a5; /* remove later */


static /*volatile*/ wait_queue_head_t P2WaitQueueHead_WaitDeque;
static /*volatile*/ wait_queue_head_t P2WaitQueueHead_WaitFrame;
static /*volatile*/ wait_queue_head_t P2WaitQueueHead_WaitFrameEQDforDQ;
static spinlock_t      SpinLock_P2FrameList;
#define _MAX_SUPPORT_P2_FRAME_NUM_ 512
#define _MAX_SUPPORT_P2_BURSTQ_NUM_ 8
#define _MAX_SUPPORT_P2_PACKAGE_NUM_ (_MAX_SUPPORT_P2_FRAME_NUM_/_MAX_SUPPORT_P2_BURSTQ_NUM_)
struct DIP_P2_BUFQUE_IDX_STRUCT {
	signed int start; /* starting index for frames in the ring list */
	signed int curr; /* current index for running frame in the ring list */
	signed int end; /* ending index for frames in the ring list */
};

struct DIP_P2_FRAME_UNIT_STRUCT {
	unsigned int               processID;  /* caller process ID */
	unsigned int               callerID;   /* caller thread ID */
	unsigned int               cqMask; /* QQ. optional -> to judge cq combination(for judging frame) */

	enum DIP_P2_BUF_STATE_ENUM  bufSts;     /* buffer status */
};

static struct DIP_P2_BUFQUE_IDX_STRUCT P2_FrameUnit_List_Idx[DIP_P2_BUFQUE_PROPERTY_NUM];
static struct DIP_P2_FRAME_UNIT_STRUCT P2_FrameUnit_List[DIP_P2_BUFQUE_PROPERTY_NUM][_MAX_SUPPORT_P2_FRAME_NUM_];

struct DIP_P2_FRAME_PACKAGE_STRUCT {
	unsigned int                processID;  /* caller process ID */
	unsigned int                callerID;   /* caller thread ID */
	unsigned int                dupCQIdx;       /* to judge it belongs to which frame package */
	signed int                   frameNum;
	signed int                   dequedNum;  /* number of dequed buffer no matter deque success or fail */
};
static struct DIP_P2_BUFQUE_IDX_STRUCT P2_FramePack_List_Idx[DIP_P2_BUFQUE_PROPERTY_NUM];
static struct DIP_P2_FRAME_PACKAGE_STRUCT
	P2_FramePackage_List[DIP_P2_BUFQUE_PROPERTY_NUM][_MAX_SUPPORT_P2_PACKAGE_NUM_];




static  spinlock_t      SpinLockRegScen;

/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32
static  spinlock_t      SpinLock_UserKey;


/*******************************************************************************
*
********************************************************************************/
/* internal data */
/* pointer to the kmalloc'd area, rounded up to a page boundary */
static int *pTbl_RTBuf[DIP_IRQ_TYPE_AMOUNT];
static int Tbl_RTBuf_MMPSize[DIP_IRQ_TYPE_AMOUNT];

/* original pointer for kmalloc'd area as returned by kmalloc */
static void *pBuf_kmalloc[DIP_IRQ_TYPE_AMOUNT];

unsigned long FIP_g_Flash_SpinLock;


static unsigned int G_u4EnableClockCount;

int DIP_pr_detect_count;

/*save ion fd*/
/*#define ENABLE_KEEP_ION_HANDLE*/

#ifdef ENABLE_KEEP_ION_HANDLE
#define _ion_keep_max_   (64)/*32*/
#include "ion_drv.h" /*g_ion_device*/
static struct ion_client *pIon_client;
/*static signed int G_WRDMA_IonCt[2][_dma_max_wr_*_ion_keep_max_] = { {0}, {0} };*/
/*static signed int G_WRDMA_IonFd[2][_dma_max_wr_*_ion_keep_max_] = { {0}, {0} };*/
/*static struct ion_handle *G_WRDMA_IonHnd[2][_dma_max_wr_*_ion_keep_max_] = { {NULL}, {NULL} };*/
/*static spinlock_t SpinLock_IonHnd[2][_dma_max_wr_]; */ /* protect G_WRDMA_IonHnd & G_WRDMA_IonFd */

struct T_ION_TBL {
	enum DIP_DEV_NODE_ENUM node;
	signed int *pIonCt;
	signed int *pIonFd;
	struct ion_handle **pIonHnd;
	spinlock_t *pLock;
};

static struct T_ION_TBL gION_TBL[DIP_DEV_NODE_NUM] = {
	{DIP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
};
#endif
/*******************************************************************************
*
********************************************************************************/
struct DIP_USER_INFO_STRUCT {
	pid_t   Pid;
	pid_t   Tid;
};

/*******************************************************************************
*
********************************************************************************/
#define DIP_BUF_SIZE            (4096)
#define DIP_BUF_SIZE_WRITE      1024
#define DIP_BUF_WRITE_AMOUNT    6

enum DIP_BUF_STATUS_ENUM {
	DIP_BUF_STATUS_EMPTY,
	DIP_BUF_STATUS_HOLD,
	DIP_BUF_STATUS_READY
};

struct DIP_BUF_STRUCT {
	enum DIP_BUF_STATUS_ENUM Status;
	unsigned int                Size;
	unsigned char *pData;
};

struct DIP_BUF_INFO_STRUCT {
	struct DIP_BUF_STRUCT      Read;
	struct DIP_BUF_STRUCT      Write[DIP_BUF_WRITE_AMOUNT];
};


/*******************************************************************************
*
********************************************************************************/
#define DIP_ISR_MAX_NUM 32
#define INT_ERR_WARN_TIMER_THREAS 1000
#define INT_ERR_WARN_MAX_TIME 1

struct DIP_IRQ_ERR_WAN_CNT_STRUCT {
	unsigned int m_err_int_cnt[DIP_IRQ_TYPE_AMOUNT][DIP_ISR_MAX_NUM]; /* cnt for each err int # */
	unsigned int m_warn_int_cnt[DIP_IRQ_TYPE_AMOUNT][DIP_ISR_MAX_NUM]; /* cnt for each warning int # */
	unsigned int m_err_int_mark[DIP_IRQ_TYPE_AMOUNT]; /* mark for err int, where its cnt > threshold */
	unsigned int m_warn_int_mark[DIP_IRQ_TYPE_AMOUNT]; /* mark for warn int, where its cnt > threshold */
	unsigned long m_int_usec[DIP_IRQ_TYPE_AMOUNT];
};

static signed int FirstUnusedIrqUserKey = 1;
#define USERKEY_STR_LEN 128

struct UserKeyInfo {
	char userName[USERKEY_STR_LEN]; /* name for the user that register a userKey */
	int userKey;    /* the user key for that user */
};
/* array for recording the user name for a specific user key */
static struct UserKeyInfo IrqUserKey_UserInfo[IRQ_USER_NUM_MAX];

struct DIP_IRQ_INFO_STRUCT {
	/* Add an extra index for status type in Everest -> signal or dma */
	unsigned int    Status[DIP_IRQ_TYPE_AMOUNT][IRQ_USER_NUM_MAX];
	unsigned int    Mask[DIP_IRQ_TYPE_AMOUNT];
};

struct DIP_TIME_LOG_STRUCT {
	unsigned int     Vd;
	unsigned int     Expdone;
	unsigned int     WorkQueueVd;
	unsigned int     WorkQueueExpdone;
	unsigned int     TaskletVd;
	unsigned int     TaskletExpdone;
};

enum _eChannel {
	_PASS1      = 0,
	_PASS1_D    = 1,
	_CAMSV      = 2,
	_CAMSV_D    = 3,
	_PASS2      = 4,
	_ChannelMax = 5,
} eChannel;



/**********************************************************************/
#define my_get_pow_idx(value)      \
	({                                                          \
		int i = 0, cnt = 0;                                  \
		for (i = 0; i < 32; i++) {                            \
			if ((value>>i) & (0x00000001)) {   \
				break;                                      \
			} else {                                            \
				cnt++;  \
			}                                      \
		}                                                    \
		cnt;                                                \
	})


#define SUPPORT_MAX_IRQ 32
struct DIP_INFO_STRUCT {
	spinlock_t                      SpinLockIspRef;
	spinlock_t                      SpinLockIsp;
	spinlock_t                      SpinLockIrq[DIP_IRQ_TYPE_AMOUNT];
	spinlock_t                      SpinLockIrqCnt[DIP_IRQ_TYPE_AMOUNT];
	spinlock_t                      SpinLockRTBC;
	spinlock_t                      SpinLockClock;
	wait_queue_head_t               WaitQueueHead[DIP_IRQ_TYPE_AMOUNT];
	/* wait_queue_head_t*              WaitQHeadList; */
	wait_queue_head_t      WaitQHeadList[SUPPORT_MAX_IRQ];
	unsigned int                         UserCount;
	unsigned int                         DebugMask;
	signed int							IrqNum;
	struct DIP_IRQ_INFO_STRUCT			IrqInfo;
	struct DIP_IRQ_ERR_WAN_CNT_STRUCT		IrqCntInfo;
	struct DIP_BUF_INFO_STRUCT			BufInfo;
	struct DIP_TIME_LOG_STRUCT             TimeLog;
};



static struct DIP_INFO_STRUCT IspInfo;
static bool    SuspnedRecord[DIP_DEV_NODE_NUM] = {0};

enum _eLOG_TYPE {
	_LOG_DBG = 0,   /* currently, only used at ipl_buf_ctrl. to protect critical section */
	_LOG_INF = 1,
	_LOG_ERR = 2,
	_LOG_MAX = 3,
} eLOG_TYPE;

enum _eLOG_OP {
	_LOG_INIT = 0,
	_LOG_RST = 1,
	_LOG_ADD = 2,
	_LOG_PRT = 3,
	_LOG_GETCNT = 4,
	_LOG_OP_MAX = 5
} eLOG_OP;

#define NORMAL_STR_LEN (512)
#define ERR_PAGE 2
#define DBG_PAGE 2
#define INF_PAGE 4
/* #define SV_LOG_STR_LEN NORMAL_STR_LEN */

#define LOG_PPNUM 2
struct SV_LOG_STR {
	unsigned int _cnt[LOG_PPNUM][_LOG_MAX];
	/* char   _str[_LOG_MAX][SV_LOG_STR_LEN]; */
	char *_str[LOG_PPNUM][_LOG_MAX];
	struct S_START_T   _lastIrqTime;
};

static void *pLog_kmalloc;
static struct SV_LOG_STR gSvLog[DIP_IRQ_TYPE_AMOUNT];

/**
 *   for irq used,keep log until IRQ_LOG_PRINTER being involked,
 *   limited:
 *   each log must shorter than 512 bytes
 *   total log length in each irq/logtype can't over 1024 bytes
 */
#define IRQ_LOG_KEEPER_T(sec, usec) {\
		ktime_t time;           \
		time = ktime_get();     \
		sec = time.tv64;        \
		do_div(sec, 1000);    \
		usec = do_div(sec, 1000000);\
	}
#if 1
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...) do {\
	char *ptr; \
	char *pDes;\
	signed int avaLen;\
	unsigned int *ptr2 = &gSvLog[irq]._cnt[ppb][logT];\
	unsigned int str_leng;\
	unsigned int i;\
	struct SV_LOG_STR *pSrc = &gSvLog[irq];\
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
	avaLen = str_leng - 1 - gSvLog[irq]._cnt[ppb][logT];\
	if (avaLen > 1) {\
		snprintf((char *)(pDes), avaLen, "[%d.%06d]" fmt,\
			gSvLog[irq]._lastIrqTime.sec, gSvLog[irq]._lastIrqTime.usec,\
			##__VA_ARGS__);   \
		if ('\0' != gSvLog[irq]._str[ppb][logT][str_leng - 1]) {\
			LOG_ERR("log str over flow(%d)", irq);\
		} \
		while (*ptr++ != '\0') {        \
			(*ptr2)++;\
		}     \
	} else { \
		LOG_INF("(%d)(%d)log str avalible=0, print log\n", irq, logT);\
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
			avaLen = str_leng - 1;\
			ptr = pDes = (char *)&(pSrc->_str[ppb][logT][pSrc->_cnt[ppb][logT]]);\
			ptr2 = &(pSrc->_cnt[ppb][logT]);\
			snprintf((char *)(pDes), avaLen, fmt, ##__VA_ARGS__);   \
			while (*ptr++ != '\0') {\
				(*ptr2)++;\
			} \
		} \
	} \
} while (0)
#else
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, args...)    pr_debug(IRQTag fmt,  ##args)
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

/* //////////////////////////////////////////////////// */
union _FBC_CTRL_1_ {
	struct { /* 0x18004110 */
		unsigned int  FBC_NUM                               :  6;      /*  0.. 5, 0x0000003F */
		unsigned int  rsv_6                                 :  9;      /*  6..14, 0x00007FC0 */
		unsigned int  FBC_EN                                :  1;      /* 15..15, 0x00008000 */
		unsigned int  FBC_MODE                              :  1;      /* 16..16, 0x00010000 */
		unsigned int  LOCK_EN                               :  1;      /* 17..17, 0x00020000 */
		unsigned int  rsv_18                                :  2;      /* 18..19, 0x000C0000 */
		unsigned int  DROP_TIMING                           :  1;      /* 20..20, 0x00100000 */
		unsigned int  rsv_21                                :  3;      /* 21..23, 0x00E00000 */
		unsigned int  SUB_RATIO                             :  8;      /* 24..31, 0xFF000000 */
	} Bits;
	unsigned int Raw;
} FBC_CTRL_1;  /* CAM_A_FBC_IMGO_CTL1 */

union _FBC_CTRL_2_ {
	struct { /* 0x18004114 */
		unsigned int  FBC_CNT                               :  7;      /*  0.. 6, 0x0000007F */
		unsigned int  rsv_7                                 :  1;      /*  7.. 7, 0x00000080 */
		unsigned int  RCNT                                  :  6;      /*  8..13, 0x00003F00 */
		unsigned int  rsv_14                                :  2;      /* 14..15, 0x0000C000 */
		unsigned int  WCNT                                  :  6;      /* 16..21, 0x003F0000 */
		unsigned int  rsv_22                                :  2;      /* 22..23, 0x00C00000 */
		unsigned int  DROP_CNT                              :  8;      /* 24..31, 0xFF000000 */
	} Bits;
	unsigned int Raw;
} FBC_CTRL_2;  /* CAM_A_FBC_IMGO_CTL2 */


struct _dip_bk_reg_t {
	unsigned int  CAM_TG_INTER_ST;                                 /* 453C*/
};

static struct _dip_bk_reg_t g_BkReg[DIP_IRQ_TYPE_AMOUNT];

/* Everest top registers */
/*#define CAMSYS_REG_CG_CON               (DIP_CAMSYS_CONFIG_BASE + 0x0)*/
#define IMGSYS_REG_CG_CON               (DIP_IMGSYS_CONFIG_BASE + 0x0)
/*#define CAMSYS_REG_CG_SET               (DIP_CAMSYS_CONFIG_BASE + 0x4)*/
#define IMGSYS_REG_CG_SET               (DIP_IMGSYS_CONFIG_BASE + 0x4)
/*#define CAMSYS_REG_CG_CLR               (DIP_CAMSYS_CONFIG_BASE + 0x8)*/
#define IMGSYS_REG_CG_CLR               (DIP_IMGSYS_CONFIG_BASE + 0x8)

#define DIP_REG_ADDR_EN1                (DIP_ADDR + 0x4)
#define DIP_REG_ADDR_INT_P1_ST          (DIP_ADDR + 0x4C)
#define CAM_REG_ADDR_DMA_ST             (DIP_ADDR + 0x4C)
#define DIP_REG_ADDR_INT_P1_ST2         (DIP_ADDR + 0x54)
#define DIP_REG_ADDR_INT_P1_ST_D        (DIP_ADDR + 0x5C)
#define DIP_REG_ADDR_INT_P1_ST2_D       (DIP_ADDR + 0x64)
#define DIP_REG_ADDR_INT_P2_ST          (DIP_ADDR + 0x6C)
#define DIP_REG_ADDR_INT_STATUSX        (DIP_ADDR + 0x70)
#define DIP_REG_ADDR_INT_STATUS2X       (DIP_ADDR + 0x74)
#define DIP_REG_ADDR_INT_STATUS3X       (DIP_ADDR + 0x78)
#define DIP_REG_ADDR_CAM_SW_CTL         (DIP_ADDR + 0x8C)

#define DIP_TPIPE_ADDR                  (0x15004000)

/* CAM_CTL_SW_CTL */
#define DIP_REG_SW_CTL_SW_RST_P1_MASK   (0x00000007)
#define DIP_REG_SW_CTL_SW_RST_TRIG      (0x00000001)
#define DIP_REG_SW_CTL_SW_RST_STATUS    (0x00000002)
#define DIP_REG_SW_CTL_HW_RST           (0x00000004)
#define DIP_REG_SW_CTL_SW_RST_P2_MASK   (0x00000070)
#define DIP_REG_SW_CTL_SW_RST_P2_TRIG   (0x00000010)
#define DIP_REG_SW_CTL_SW_RST_P2_STATUS (0x00000020)
#define DIP_REG_SW_CTL_HW_RST_P2        (0x00000040)

/* if dip has been suspend, frame cnt needs to add previous value*/
#define DIP_RD32_TG_CAM_FRM_CNT(IrqType, reg_module) ({\
	unsigned int _regVal;\
	_regVal = DIP_RD32(CAM_REG_TG_INTER_ST(reg_module));\
	_regVal = ((_regVal & 0x00FF0000) >> 16) + g_BkReg[IrqType].CAM_TG_INTER_ST;\
	if (_regVal > 255) { \
		_regVal -= 256;\
	} \
	_regVal;\
})

#if 0
/*******************************************************************************
* file shrink
********************************************************************************/
#include "camera_dip_reg.c"
#include "camera_dip_isr.c"
#endif

/*******************************************************************************
*
********************************************************************************/
static inline unsigned int DIP_MsToJiffies(unsigned int Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline unsigned int DIP_UsToJiffies(unsigned int Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}

/*******************************************************************************
*
********************************************************************************/
static inline unsigned int DIP_JiffiesToMs(unsigned int Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}

/*******************************************************************************
*
********************************************************************************/

#define RegDump(start, end) {\
	unsigned int i;\
	for (i = start; i <= end; i += 0x10) {\
		LOG_INF("QQ [0x%08X %08X],[0x%08X %08X],[0x%08X %08X],[0x%08X %08X]\n",\
			(unsigned int)(DIP_TPIPE_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i),\
			(unsigned int)(DIP_TPIPE_ADDR + i+0x4), (unsigned int)DIP_RD32(DIP_ADDR + i+0x4),\
			(unsigned int)(DIP_TPIPE_ADDR + i+0x8), (unsigned int)DIP_RD32(DIP_ADDR + i+0x8),\
			(unsigned int)(DIP_TPIPE_ADDR + i+0xc), (unsigned int)DIP_RD32(DIP_ADDR + i+0xc));\
	} \
}

static signed int DIP_DumpDIPReg(void)
{
	signed int Ret = 0;
	unsigned int i, cmdqidx = 0;
#ifdef AEE_DUMP_REDUCE_MEMORY
	unsigned int offset = 0;
	uintptr_t OffsetAddr = 0;
	unsigned int ctrl_start;
#else
	unsigned int offset = 0;
	uintptr_t OffsetAddr = 0;
	unsigned int ctrl_start;
#endif
	/*  */
	LOG_INF("- E.");
	LOG_INF("g_bDumpPhyDIPBuf:(0x%x), g_pPhyDIPBuffer:(0x%p), g_bIonBuf:(0x%x)\n", g_bDumpPhyDIPBuf,
	g_pPhyDIPBuffer, g_bIonBufferAllocated);
#ifdef AEE_DUMP_REDUCE_MEMORY
	if (g_bDumpPhyDIPBuf == MFALSE) {
		ctrl_start = DIP_RD32(DIP_DIP_A_BASE + 0x0000);
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pPhyDIPBuffer != NULL) {
				LOG_DBG("g_pPhyDIPBuffer is not NULL:(0x%pK)\n", g_pPhyDIPBuffer);
				vfree(g_pPhyDIPBuffer);
				g_pPhyDIPBuffer = NULL;
			}
			g_pPhyDIPBuffer = vmalloc(DIP_DIP_REG_SIZE);
			if (g_pPhyDIPBuffer == NULL)
				LOG_DBG("ERROR: g_pPhyDIPBuffer kmalloc failed\n");

			if (g_pKWTpipeBuffer != NULL) {
				LOG_DBG("g_pKWTpipeBuffer is not NULL:(0x%pK)\n", g_pKWTpipeBuffer);
				vfree(g_pKWTpipeBuffer);
				g_pKWTpipeBuffer = NULL;
			}
			g_pKWTpipeBuffer = vmalloc(MAX_DIP_TILE_TDR_HEX_NO);
			if (g_pKWTpipeBuffer == NULL)
				LOG_DBG("ERROR: g_pKWTpipeBuffer kmalloc failed\n");

			if (g_pKWCmdqBuffer != NULL) {
				LOG_DBG("g_KWCmdqBuffer is not NULL:(0x%pK)\n", g_pKWCmdqBuffer);
				vfree(g_pKWCmdqBuffer);
				g_pKWCmdqBuffer = NULL;
			}
			g_pKWCmdqBuffer = vmalloc(MAX_DIP_CMDQ_BUFFER_SIZE);
			if (g_pKWCmdqBuffer == NULL)
				LOG_DBG("ERROR: g_KWCmdqBuffer kmalloc failed\n");

			if (g_pKWVirDIPBuffer != NULL) {
				LOG_DBG("g_KWVirDIPBuffer is not NULL:(0x%pK)\n", g_pKWVirDIPBuffer);
				vfree(g_pKWVirDIPBuffer);
				g_pKWVirDIPBuffer = NULL;
			}
			g_pKWVirDIPBuffer = vmalloc(DIP_DIP_REG_SIZE);
			if (g_pKWVirDIPBuffer == NULL)
				LOG_DBG("ERROR: g_KWVirDIPBuffer kmalloc failed\n");
		}

		if (g_pPhyDIPBuffer != NULL) {
			for (i = 0; i < (DIP_DIP_PHYSICAL_REG_SIZE >> 2); i = i + 4) {
				g_pPhyDIPBuffer[i] = DIP_RD32(DIP_DIP_A_BASE + (i*4));
				g_pPhyDIPBuffer[i+1] = DIP_RD32(DIP_DIP_A_BASE + ((i+1)*4));
				g_pPhyDIPBuffer[i+2] = DIP_RD32(DIP_DIP_A_BASE + ((i+2)*4));
				g_pPhyDIPBuffer[i+3] = DIP_RD32(DIP_DIP_A_BASE + ((i+3)*4));
			}
		} else {
			LOG_INF("g_pPhyDIPBuffer:(0x%pK)\n", g_pPhyDIPBuffer);
		}
		g_dumpInfo.tdri_baseaddr = DIP_RD32(DIP_DIP_A_BASE + 0x204);/* 0x15022204 */
		g_dumpInfo.imgi_baseaddr = DIP_RD32(DIP_DIP_A_BASE + 0x400);/* 0x15022400 */
		g_dumpInfo.dmgi_baseaddr = DIP_RD32(DIP_DIP_A_BASE + 0x520);/* 0x15022520 */
		g_tdriaddr = g_dumpInfo.tdri_baseaddr;
		for (cmdqidx = 0; cmdqidx < 32 ; cmdqidx++) {
			if (ctrl_start & (0x1<<cmdqidx)) {
				g_cmdqaddr = DIP_RD32(DIP_DIP_A_BASE + 0x108 + (cmdqidx*12));
				break;
			}
		}
		if ((g_TpipeBaseAddrInfo.MemPa != 0) && (g_TpipeBaseAddrInfo.MemVa != NULL)
			&& (g_pKWTpipeBuffer != NULL)) {
			/* to get frame tdri baseaddress, otherwide  you possible get one of the tdr bade addr*/
			offset = ((g_tdriaddr & (~(g_TpipeBaseAddrInfo.MemSizeDiff-1)))-g_TpipeBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_TpipeBaseAddrInfo.MemVa)+offset;
			if (copy_from_user(g_pKWTpipeBuffer, (void __user *)(OffsetAddr),
				MAX_DIP_TILE_TDR_HEX_NO) != 0) {
				LOG_ERR("cpy tpipe fail. tdriaddr:0x%x, MemVa:0x%lx,MemPa:0x%x, offset:0x%x\n",
				g_tdriaddr, (uintptr_t)g_TpipeBaseAddrInfo.MemVa, g_TpipeBaseAddrInfo.MemPa, offset);
			}
		}
		LOG_INF("tdraddr:0x%x,MemVa:0x%lx,MemPa:0x%x,MemSizeDiff:0x%x,offset:0x%x,g_pKWTpipeBuffer:0x%pK\n",
		g_tdriaddr, (uintptr_t)g_TpipeBaseAddrInfo.MemVa, g_TpipeBaseAddrInfo.MemPa,
		g_TpipeBaseAddrInfo.MemSizeDiff, offset, g_pKWTpipeBuffer);
		if ((g_CmdqBaseAddrInfo.MemPa != 0) && (g_CmdqBaseAddrInfo.MemVa != NULL)
				&& (g_pKWCmdqBuffer != NULL) && (g_pKWVirDIPBuffer != NULL)) {
			offset = (g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa)+(g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			if (copy_from_user(g_pKWCmdqBuffer, (void __user *)(OffsetAddr),
				MAX_DIP_CMDQ_BUFFER_SIZE) != 0) {
				LOG_ERR("cpy cmdq fail. cmdqaddr:0x%x, MemVa:0x%lx, MemPa:0x%x, offset:0x%x\n",
					g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa,
					g_CmdqBaseAddrInfo.MemPa, offset);
			}
			LOG_INF("cmdqidx:0x%x, cmdqaddr:0x%x, MemVa:0x%lx, MemPa:0x%x, offset:0x%x\n",
			cmdqidx, g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa, offset);
			offset = offset+g_CmdqBaseAddrInfo.MemSizeDiff;
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa)+offset;
			if (copy_from_user(g_pKWVirDIPBuffer, (void __user *)(OffsetAddr),
				DIP_DIP_REG_SIZE) != 0) {
				LOG_ERR("cpy vir dip fail.cmdqaddr:0x%x,MVa:0x%lx,MPa:0x%x,MSzDiff:0x%x,offset:0x%x\n",
				g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa,
				g_CmdqBaseAddrInfo.MemSizeDiff, offset);
			}
			LOG_INF("cmdqaddr:0x%x,MVa:0x%lx,MPa:0x%x,MSzDiff:0x%x\n",
			g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa,
			g_CmdqBaseAddrInfo.MemSizeDiff);
			LOG_INF("ofset:0x%x,KWCmdBuf:0x%pK,KWTdrBuf:0x%pK\n",
			offset, g_pKWCmdqBuffer, g_pKWTpipeBuffer);
		} else {
			LOG_INF("cmdqadd:0x%x,MVa:0x%lx,MPa:0x%x,MSzDiff:0x%x,KWCmdBuf:0x%pK,KWTdrBuf:0x%pK\n",
			g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa,
			g_CmdqBaseAddrInfo.MemSizeDiff, g_pKWCmdqBuffer, g_pKWTpipeBuffer);
		}
		g_bDumpPhyDIPBuf = MTRUE;
	}

#else
		if (g_bDumpPhyDIPBuf == MFALSE) {
			ctrl_start = DIP_RD32(DIP_DIP_A_BASE + 0x0000);
			if (g_pPhyDIPBuffer != NULL) {
				for (i = 0; i < (DIP_DIP_PHYSICAL_REG_SIZE >> 2); i = i + 4) {
					g_pPhyDIPBuffer[i] = DIP_RD32(DIP_DIP_A_BASE + (i*4));
					g_pPhyDIPBuffer[i+1] = DIP_RD32(DIP_DIP_A_BASE + ((i+1)*4));
				g_pPhyDIPBuffer[i+2] = DIP_RD32(DIP_DIP_A_BASE + ((i+2)*4));
				g_pPhyDIPBuffer[i+3] = DIP_RD32(DIP_DIP_A_BASE + ((i+3)*4));
			}
		} else {
			LOG_INF("g_pPhyDIPBuffer:(0x%pK)\n", g_pPhyDIPBuffer);
		}
		g_dumpInfo.tdri_baseaddr = DIP_RD32(DIP_DIP_A_BASE + 0x204);/* 0x15022204 */
		g_dumpInfo.imgi_baseaddr = DIP_RD32(DIP_DIP_A_BASE + 0x400);/* 0x15022400 */
		g_dumpInfo.dmgi_baseaddr = DIP_RD32(DIP_DIP_A_BASE + 0x520);/* 0x15022520 */
		g_tdriaddr = g_dumpInfo.tdri_baseaddr;
		for (cmdqidx = 0; cmdqidx < 32 ; cmdqidx++) {
			if (ctrl_start & (0x1<<cmdqidx)) {
				g_cmdqaddr = DIP_RD32(DIP_DIP_A_BASE + 0x108 + (cmdqidx*12));
				break;
			}
		}
		if ((g_TpipeBaseAddrInfo.MemPa != 0) && (g_TpipeBaseAddrInfo.MemVa != NULL)) {
			/* to get frame tdri baseaddress, otherwide  you possible get one of the tdr bade addr*/
			offset = ((g_tdriaddr & (~(g_TpipeBaseAddrInfo.MemSizeDiff-1)))-g_TpipeBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_TpipeBaseAddrInfo.MemVa)+offset;
			if (copy_from_user(g_KWTpipeBuffer, (void __user *)(OffsetAddr),
				MAX_DIP_TILE_TDR_HEX_NO) != 0) {
				LOG_ERR("cpy tpipe fail.tdriaddr:0x%x,MVa:0x%lx,MPa:0x%x,ofset:0x%x\n",
				g_tdriaddr, (uintptr_t)g_TpipeBaseAddrInfo.MemVa, g_TpipeBaseAddrInfo.MemPa, offset);
			}
			LOG_INF("tdriaddr:0x%x, MemVa:0x%lx, MemPa:0x%x, MemSizeDiff:0x%x, offset:0x%x\n",
			g_tdriaddr, (uintptr_t)g_TpipeBaseAddrInfo.MemVa, g_TpipeBaseAddrInfo.MemPa,
			g_TpipeBaseAddrInfo.MemSizeDiff, offset);
		}
		if ((g_CmdqBaseAddrInfo.MemPa != 0) && (g_CmdqBaseAddrInfo.MemVa != NULL)) {
			offset = (g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa)+(g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			if (copy_from_user(g_KWCmdqBuffer, (void __user *)(OffsetAddr),
				MAX_DIP_CMDQ_BUFFER_SIZE) != 0) {
				LOG_ERR("cpy cmdq fail. cmdqaddr:0x%x, MemVa:0x%lx, MemPa:0x%x, offset:0x%x\n",
				g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa, offset);
			}
			LOG_INF("cmdqidx:0x%x, cmdqaddr:0x%x, MemVa:0x%lx, MemPa:0x%x, offset:0x%x\n",
			cmdqidx, g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa, offset);
			offset = offset+g_CmdqBaseAddrInfo.MemSizeDiff;
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa)+offset;
			if (copy_from_user(g_KWVirDIPBuffer, (void __user *)(OffsetAddr),
				DIP_DIP_REG_SIZE) != 0) {
				LOG_ERR("cpy vir dip fail.cmdqaddr:0x%x,MVa:0x%lx,MPa:0x%x,MSzDiff:0x%x,ofset:0x%x\n",
				g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa,
				g_CmdqBaseAddrInfo.MemSizeDiff, offset);
			}
			LOG_INF("cmdqaddr:0x%x, MemVa:0x%lx, MemPa:0x%x, MemSizeDiff:0x%x, offset:0x%x\n",
			g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa, g_CmdqBaseAddrInfo.MemPa,
			g_CmdqBaseAddrInfo.MemSizeDiff, offset);
		}
		g_bDumpPhyDIPBuf = MTRUE;
	}
#endif

	LOG_INF("direct link:g_bDumpPhyDIPBuf:(0x%x),cmdqidx(0x%x),cmdqaddr(0x%x),tdriaddr(0x%x)\n",
		g_bDumpPhyDIPBuf, cmdqidx, g_cmdqaddr, g_tdriaddr);
	LOG_INF("dip: 15022000(0x%x)-15022004(0x%x)-15022008(0x%x)-1502200C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0000), DIP_RD32(DIP_DIP_A_BASE + 0x0004),
		DIP_RD32(DIP_DIP_A_BASE + 0x0008), DIP_RD32(DIP_DIP_A_BASE + 0x000C));
	LOG_INF("dip: 15022010(0x%x)-15022014(0x%x)-15022018(0x%x)-1502201C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0010), DIP_RD32(DIP_DIP_A_BASE + 0x0014),
		DIP_RD32(DIP_DIP_A_BASE + 0x0018), DIP_RD32(DIP_DIP_A_BASE + 0x001C));
	LOG_INF("dip: 15022D20(0x%x)-15022D24(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0D20), DIP_RD32(DIP_DIP_A_BASE + 0x0D24));
	LOG_INF("dip: 15022408(0x%x)-15022204(0x%x)-15022208(0x%x)-1502220C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0408), DIP_RD32(DIP_DIP_A_BASE + 0x0204),
		DIP_RD32(DIP_DIP_A_BASE + 0x0208), DIP_RD32(DIP_DIP_A_BASE + 0x020C));
	LOG_INF("dip: 15022050(0x%x)-15022054(0x%x)-15022058(0x%x)-1502205C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0050), DIP_RD32(DIP_DIP_A_BASE + 0x0054),
		DIP_RD32(DIP_DIP_A_BASE + 0x0058), DIP_RD32(DIP_DIP_A_BASE + 0x005C));
	LOG_INF("dip: 150220B8(0x%x)-150220BC(0x%x)-150220C0(0x%x)-150220C4(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x00B8), DIP_RD32(DIP_DIP_A_BASE + 0x00BC),
		DIP_RD32(DIP_DIP_A_BASE + 0x00C0), DIP_RD32(DIP_DIP_A_BASE + 0x00C4));
	LOG_INF("dip: 150220C8(0x%x)-150220CC(0x%x)-150220D0(0x%x)-150220D4(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x00C8), DIP_RD32(DIP_DIP_A_BASE + 0x00CC),
		DIP_RD32(DIP_DIP_A_BASE + 0x00D0), DIP_RD32(DIP_DIP_A_BASE + 0x00D4));
	LOG_INF("dip: 15022104(0x%x)-15022108(0x%x)-1502211C(0x%x)-15022120(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0104), DIP_RD32(DIP_DIP_A_BASE + 0x0108),
		DIP_RD32(DIP_DIP_A_BASE + 0x011C), DIP_RD32(DIP_DIP_A_BASE + 0x0120));
	LOG_INF("dip: 15022128(0x%x)-1502212C(0x%x)-15022134(0x%x)-15022138(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0128), DIP_RD32(DIP_DIP_A_BASE + 0x012C),
		DIP_RD32(DIP_DIP_A_BASE + 0x0134), DIP_RD32(DIP_DIP_A_BASE + 0x0138));
	LOG_INF("dip: 15022140(0x%x)-15022144(0x%x)-1502214C(0x%x)-15022150(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0140), DIP_RD32(DIP_DIP_A_BASE + 0x0144),
		DIP_RD32(DIP_DIP_A_BASE + 0x014C), DIP_RD32(DIP_DIP_A_BASE + 0x0150));
	LOG_INF("dip: 15022158(0x%x)-1502215C(0x%x)-15022164(0x%x)-15022168(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0158), DIP_RD32(DIP_DIP_A_BASE + 0x015C),
		DIP_RD32(DIP_DIP_A_BASE + 0x0164), DIP_RD32(DIP_DIP_A_BASE + 0x0168));
	LOG_INF("dip: 15022170(0x%x)-15022174(0x%x)-1502217C(0x%x)-15022180(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0170), DIP_RD32(DIP_DIP_A_BASE + 0x0174),
		DIP_RD32(DIP_DIP_A_BASE + 0x017C), DIP_RD32(DIP_DIP_A_BASE + 0x0180));
	LOG_INF("dip: 15022188(0x%x)-1502218C(0x%x)-15022194(0x%x)-15022198(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0188), DIP_RD32(DIP_DIP_A_BASE + 0x018C),
		DIP_RD32(DIP_DIP_A_BASE + 0x0194), DIP_RD32(DIP_DIP_A_BASE + 0x0198));
	LOG_INF("dip: 15022060(0x%x)-15022064(0x%x)-15022068(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0060), DIP_RD32(DIP_DIP_A_BASE + 0x0064),
		DIP_RD32(DIP_DIP_A_BASE + 0x0068));
		/* 0080, 0x15022080, DIP_A_CTL_DBG_SET */
		DIP_WR32(DIP_DIP_A_BASE + 0x80, 0x0);
		/* 06BC, 0x150226BC, DIP_A_DMA_DEBUG_SEL */
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x000013);
		/* 0084, 0x15022084, DIP_A_CTL_DBG_PORT */
		LOG_INF("0x000013 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x000113);
		LOG_INF("0x000113 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x000213);
		LOG_INF("0x000213 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x000313);
		LOG_INF("0x000313 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		/* IMG2O */
		/* 06BC, 0x150226BC, DIP_A_DMA_DEBUG_SEL */
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001013);
		/* 0084, 0x15022084, DIP_A_CTL_DBG_PORT */
		LOG_INF("0x001013 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001113);
		LOG_INF("0x001113 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001213);
		LOG_INF("0x001213 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001313);
		LOG_INF("0x001313 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		/*IMG3O */
		/* 06BC, 0x150226BC, DIP_A_DMA_DEBUG_SEL */
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001813);
		/* 0084, 0x15022084, DIP_A_CTL_DBG_PORT */
		LOG_INF("0x001813 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001913);
		LOG_INF("0x001913 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001A13);
		LOG_INF("0x001A13 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x6BC, 0x001B13);
		LOG_INF("0x001B13 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x080, 0x003016);
		LOG_INF("0x003016 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x080, 0x003017);
		LOG_INF("0x003017 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x080, 0x003018);
		LOG_INF("0x003018 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x080, 0x003019);
		LOG_INF("0x003019 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		DIP_WR32(DIP_DIP_A_BASE + 0x080, 0x005100);
		LOG_INF("0x005100 : dip: 0x15022084(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x084));
		/* DMA Error */
		LOG_INF("img2o  0x15022644(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x644));
		LOG_INF("img2bo 0x15022648(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x648));
		LOG_INF("img3o  0x1502264C(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x64C));
		LOG_INF("img3bo 0x15022650(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x650));
		LOG_INF("img3Co 0x15022654(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x654));
		LOG_INF("feo    0x15022658(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x658));
		LOG_INF("mfbo   0x1502265C(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x65C));
		LOG_INF("imgi   0x15022660(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x660));
		LOG_INF("imgbi  0x15022664(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x664));
		LOG_INF("imgci  0x15022668(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x668));
		LOG_INF("vipi   0x1502266c(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x66c));
		LOG_INF("vip2i  0x15022670(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x670));
		LOG_INF("vip3i  0x15022674(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x674));
		LOG_INF("dmgi   0x15022678(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x678));
		LOG_INF("depi   0x1502267c(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x67C));
		LOG_INF("lcei   0x15022680(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x680));
		LOG_INF("ufdi   0x15022684(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x684));
		LOG_INF("CTL_INT_STATUSX      0x15022040(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x040));
		LOG_INF("CTL_CQ_INT_STATUSX   0x15022044(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x044));
		LOG_INF("CTL_CQ_INT2_STATUSX  0x15022048(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x048));
		LOG_INF("CTL_CQ_INT3_STATUSX  0x1502204C(0x%x)", DIP_RD32(DIP_DIP_A_BASE + 0x04C));
	LOG_INF("img3o: 0x15022290(0x%x)-0x15022298(0x%x)-0x150222A0(0x%x)-0x150222A4(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x290), DIP_RD32(DIP_DIP_A_BASE + 0x0298),
		DIP_RD32(DIP_DIP_A_BASE + 0x2A0), DIP_RD32(DIP_DIP_A_BASE + 0x02A4));
	LOG_INF("img3o: 0x150222A8(0x%x)-0x150222AC(0x%x)-0x150222B0(0x%x)-0x150222B4(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x02A8), DIP_RD32(DIP_DIP_A_BASE + 0x02AC),
		DIP_RD32(DIP_DIP_A_BASE + 0x02B0), DIP_RD32(DIP_DIP_A_BASE + 0x02B4));
	LOG_INF("img3o: 0x150222B8(0x%x)\n", DIP_RD32(DIP_DIP_A_BASE + 0x02B8));

	LOG_INF("imgi: 0x15022400(0x%x)-0x15022408(0x%x)-0x15022410(0x%x)-0x15022414(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x400), DIP_RD32(DIP_DIP_A_BASE + 0x408),
		DIP_RD32(DIP_DIP_A_BASE + 0x410), DIP_RD32(DIP_DIP_A_BASE + 0x414));
	LOG_INF("imgi: 0x15022418(0x%x)-0x1502241C(0x%x)-0x15022420(0x%x)-0x15022424(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x418), DIP_RD32(DIP_DIP_A_BASE + 0x41C),
		DIP_RD32(DIP_DIP_A_BASE + 0x420), DIP_RD32(DIP_DIP_A_BASE + 0x424));

	LOG_INF("mfbo: 0x15022350(0x%x)-0x15022358(0x%x)-0x15022360(0x%x)-0x15022364(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x350), DIP_RD32(DIP_DIP_A_BASE + 0x358),
		DIP_RD32(DIP_DIP_A_BASE + 0x360), DIP_RD32(DIP_DIP_A_BASE + 0x364));
	LOG_INF("mfbo: 0x15022368(0x%x)-0x1502236C(0x%x)-0x15022370(0x%x)-0x15022374(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x368), DIP_RD32(DIP_DIP_A_BASE + 0x36C),
		DIP_RD32(DIP_DIP_A_BASE + 0x370), DIP_RD32(DIP_DIP_A_BASE + 0x374));
	LOG_INF("mfbo: 0x15022378(0x%x)\n", DIP_RD32(DIP_DIP_A_BASE + 0x378));

	LOG_INF("img2o: 0x15022230(0x%x)-0x15022238(0x%x)-0x15022240(0x%x)-0x15022244(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x230), DIP_RD32(DIP_DIP_A_BASE + 0x238),
		DIP_RD32(DIP_DIP_A_BASE + 0x240), DIP_RD32(DIP_DIP_A_BASE + 0x244));
	LOG_INF("img2o: 0x15022248(0x%x)-0x1502224C(0x%x)-0x15022250(0x%x)-0x15022254(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0248), DIP_RD32(DIP_DIP_A_BASE + 0x024C),
		DIP_RD32(DIP_DIP_A_BASE + 0x0250), DIP_RD32(DIP_DIP_A_BASE + 0x0254));
	LOG_INF("img2o: 0x15022258(0x%x)\n", DIP_RD32(DIP_DIP_A_BASE + 0x0258));

	LOG_INF("lcei: 0x15022580(0x%x)-0x15022588(0x%x)-0x15022590(0x%x)-0x15022594(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x580), DIP_RD32(DIP_DIP_A_BASE + 0x588),
		DIP_RD32(DIP_DIP_A_BASE + 0x590), DIP_RD32(DIP_DIP_A_BASE + 0x594));
	LOG_INF("lcei: 0x15022598(0x%x)-0x1502259C(0x%x)-0x150225A0(0x%x)-0x150225A4(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0598), DIP_RD32(DIP_DIP_A_BASE + 0x059C),
		DIP_RD32(DIP_DIP_A_BASE + 0x05A0), DIP_RD32(DIP_DIP_A_BASE + 0x05A4));

	LOG_INF("crz: 0x15022C10(0x%x)-0x15022C14(0x%x)-0x15022C18(0x%x)-0x15022C1C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0xC10), DIP_RD32(DIP_DIP_A_BASE + 0xC14),
		DIP_RD32(DIP_DIP_A_BASE + 0xC18), DIP_RD32(DIP_DIP_A_BASE + 0xC1C));
	LOG_INF("crz: 0x15022C20(0x%x)-0x15022C24(0x%x)-0x15022C28(0x%x)-0x15022C2C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0C20), DIP_RD32(DIP_DIP_A_BASE + 0x0C24),
		DIP_RD32(DIP_DIP_A_BASE + 0x0C28), DIP_RD32(DIP_DIP_A_BASE + 0x0C2C));
	LOG_INF("crz: 0x15022C30(0x%x)-0x15022C34(0x%x)-0x15022C38(0x%x)-0x15022C3C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0C30), DIP_RD32(DIP_DIP_A_BASE + 0x0C34),
		DIP_RD32(DIP_DIP_A_BASE + 0x0C38), DIP_RD32(DIP_DIP_A_BASE + 0x0C3C));
	LOG_INF("crz: 0x15022C40(0x%x)-0x15022C44(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0C40), DIP_RD32(DIP_DIP_A_BASE + 0x0C44));

	LOG_INF("mfb: 0x15022F60(0x%x)-0x15022F64(0x%x)-0x15022F68(0x%x)-0x15022F6C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0xF60), DIP_RD32(DIP_DIP_A_BASE + 0xF64),
		DIP_RD32(DIP_DIP_A_BASE + 0xF68), DIP_RD32(DIP_DIP_A_BASE + 0xF6C));
	LOG_INF("mfb: 0x15022F70(0x%x)-0x15022F74(0x%x)-0x15022F78(0x%x)-0x15022F7C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0F70), DIP_RD32(DIP_DIP_A_BASE + 0x0F74),
		DIP_RD32(DIP_DIP_A_BASE + 0x0F78), DIP_RD32(DIP_DIP_A_BASE + 0x0F7C));
	LOG_INF("mfb: 0x15022F80(0x%x)-0x15022F84(0x%x)-0x15022F88(0x%x)-0x15022F8C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0F80), DIP_RD32(DIP_DIP_A_BASE + 0x0F84),
		DIP_RD32(DIP_DIP_A_BASE + 0x0F88), DIP_RD32(DIP_DIP_A_BASE + 0x0F8C));
	LOG_INF("mfb: 0x15022F90(0x%x)-0x15022F94(0x%x)-0x15022F98(0x%x)-0x15022F9C(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0F90), DIP_RD32(DIP_DIP_A_BASE + 0x0F94),
		DIP_RD32(DIP_DIP_A_BASE + 0x0F98), DIP_RD32(DIP_DIP_A_BASE + 0x0F9C));
	LOG_INF("mfb: 0x15022FA0(0x%x)-0x15022FA4(0x%x)-0x15022FA8(0x%x)-0x15022FAC(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0FA0), DIP_RD32(DIP_DIP_A_BASE + 0x0FA4),
		DIP_RD32(DIP_DIP_A_BASE + 0x0FA8), DIP_RD32(DIP_DIP_A_BASE + 0x0FAC));
	LOG_INF("mfb: 0x15022FB0(0x%x)-0x15022FB4(0x%x)-0x15022FB8(0x%x)\n",
		DIP_RD32(DIP_DIP_A_BASE + 0x0FB0), DIP_RD32(DIP_DIP_A_BASE + 0x0FB4),
		DIP_RD32(DIP_DIP_A_BASE + 0x0FB8));

	LOG_DBG("- X.");
	/*  */
	return Ret;
}

#ifndef CONFIG_MTK_CLKMGR /*CCF*/

static inline void Prepare_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order: CG_SCP_SYS_DIS-> CG_DDIP0_SMI_COMMON -> CG_SCP_SYS_DIP/CAM -> DIP clk */

	/* enable through smi API */
	smi_bus_enable(SMI_LARB_IMGSYS1, DIP_DEV_NAME);

	ret = clk_prepare_enable(dip_clk.DIP_IMG_LARB5);
	if (ret)
		LOG_ERR("cannot prepare and enable DIP_IMG_LARB5 clock\n");

	ret = clk_prepare_enable(dip_clk.DIP_IMG_LARB2);
	if (ret)
		LOG_ERR("cannot prepare and enable DIP_IMG_LARB2 clock\n");

	ret = clk_prepare_enable(dip_clk.DIP_IMG_DIP);
	if (ret)
		LOG_ERR("cannot prepare and enable DIP_IMG_DIP clock\n");

}

static inline void Disable_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order: DIP clk -> CG_SCP_SYS_DIP/CAM -> CG_DDIP0_SMI_COMMON -> CG_SCP_SYS_DIS */
	clk_disable_unprepare(dip_clk.DIP_IMG_DIP);
	clk_disable_unprepare(dip_clk.DIP_IMG_LARB2);
	clk_disable_unprepare(dip_clk.DIP_IMG_LARB5);
	smi_bus_disable(SMI_LARB_IMGSYS1, DIP_DEV_NAME);
}


#endif

/*******************************************************************************
*
********************************************************************************/
static void DIP_EnableClock(bool En)
{
#if defined(EP_NO_CLKMGR)
	unsigned int setReg;
#endif

	if (En) {
#if defined(EP_NO_CLKMGR)
		spin_lock(&(IspInfo.SpinLockClock));
		/* LOG_DBG("Camera clock enbled. G_u4EnableClockCount: %d.", G_u4EnableClockCount); */
		switch (G_u4EnableClockCount) {
		case 0:
			/* Enable clock by hardcode*/
			setReg = 0xFFFFFFFF;
			/*DIP_WR32(CAMSYS_REG_CG_CLR, setReg);*/
			/*DIP_WR32(IMGSYS_REG_CG_CLR, setReg);*/
			break;
		default:
			break;
		}
		G_u4EnableClockCount++;
		spin_unlock(&(IspInfo.SpinLockClock));
#else/*CCF*/
		/*LOG_INF("CCF:prepare_enable clk");*/
		spin_lock(&(IspInfo.SpinLockClock));
		if (G_u4EnableClockCount == 0) {
			unsigned int _reg = DIP_RD32(DIP_CLOCK_CELL_BASE);

			DIP_WR32(DIP_CLOCK_CELL_BASE, _reg|(1<<6));
		}
		G_u4EnableClockCount++;
		spin_unlock(&(IspInfo.SpinLockClock));
		Prepare_Enable_ccf_clock(); /* !!cannot be used in spinlock!! */
#endif
	} else {                /* Disable clock. */
#if defined(EP_NO_CLKMGR)
		spin_lock(&(IspInfo.SpinLockClock));
		/* LOG_DBG("Camera clock disabled. G_u4EnableClockCount: %d.", G_u4EnableClockCount); */
		G_u4EnableClockCount--;
		switch (G_u4EnableClockCount) {
		case 0:
			/* Disable clock by hardcode:
			* 1. CAMSYS_CG_SET (0x1A000004) = 0xffffffff;
			* 2. IMG_CG_SET (0x15000004) = 0xffffffff;
			*/
			setReg = 0xFFFFFFFF;
			/*DIP_WR32(CAMSYS_REG_CG_SET, setReg);*/
			/*DIP_WR32(IMGSYS_REG_CG_SET, setReg);*/
			break;
		default:
			break;
		}
		spin_unlock(&(IspInfo.SpinLockClock));
#else
		/*LOG_INF("CCF:disable_unprepare clk\n");*/
		spin_lock(&(IspInfo.SpinLockClock));
		G_u4EnableClockCount--;
		spin_unlock(&(IspInfo.SpinLockClock));
		Disable_Unprepare_ccf_clock(); /* !!cannot be used in spinlock!! */
#endif
	}
}



/*******************************************************************************
*
********************************************************************************/
static inline void DIP_Reset(signed int module)
{
	/*    unsigned int Reg;*/
	/*    unsigned int setReg;*/

	LOG_DBG("- E.\n");

	LOG_DBG(" Reset module(%d), IMGSYS clk gate(0x%x)\n",
		module, DIP_RD32(IMGSYS_REG_CG_CON));

	switch (module) {
	case DIP_DIP_A_IDX: {

		/* Reset DIP flow */

		break;
	}
	default:
		LOG_ERR("Not support reset module:%d\n", module);
		break;
	}
}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_ReadReg(struct DIP_REG_IO_STRUCT *pRegIo)
{
	unsigned int i;
	signed int Ret = 0;

	unsigned int module;
	void __iomem *regBase;


	/*  */
	struct DIP_REG_STRUCT reg;
	/* unsigned int* pData = (unsigned int*)pRegIo->Data; */
	struct DIP_REG_STRUCT *pData = (struct DIP_REG_STRUCT *)pRegIo->pData;

	module = pData->module;

	switch (module) {
	case DIP_DIP_A_IDX:
		regBase = DIP_DIP_A_BASE;
		break;
	default:
		LOG_ERR("Unsupported module(%x) !!!\n", module);
		Ret = -EFAULT;
		goto EXIT;
	}


	for (i = 0; i < pRegIo->Count; i++) {
		if (get_user(reg.Addr, (unsigned int *)&pData->Addr) != 0) {
			LOG_ERR("get_user failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		/*  */
		if (((regBase + reg.Addr) < (regBase + PAGE_SIZE))) {
			reg.Val = DIP_RD32(regBase + reg.Addr);
		} else {
			LOG_ERR("Wrong address(0x%lx)\n", (unsigned long)(regBase + reg.Addr));
			reg.Val = 0;
		}
		/*  */
		/* printk("[KernelRDReg]addr(0x%x),value()0x%x\n",DIP_ADDR_CAMINF + reg.Addr,reg.Val); */

		if (put_user(reg.Val, (unsigned int *) &(pData->Val)) != 0) {
			LOG_ERR("put_user failed\n");
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
/* Note: Can write sensor's test model only, if need write to other modules, need modify current code flow */
static signed int DIP_WriteRegToHw(
	struct DIP_REG_STRUCT *pReg,
	unsigned int         Count)
{
	signed int Ret = 0;
	unsigned int i;
	bool dbgWriteReg;
	unsigned int module;
	void __iomem *regBase;

	/* Use local variable to store IspInfo.DebugMask & DIP_DBG_WRITE_REG for saving lock time*/
	spin_lock(&(IspInfo.SpinLockIsp));
	dbgWriteReg = IspInfo.DebugMask & DIP_DBG_WRITE_REG;
	spin_unlock(&(IspInfo.SpinLockIsp));

	module = pReg->module;


	switch (module) {
	case DIP_DIP_A_IDX:
		regBase = DIP_DIP_A_BASE;
		break;
	default:
		LOG_ERR("Unsupported module(%x) !!!\n", module);
		return -EFAULT;
	}

	/*  */
	if (dbgWriteReg)
		LOG_DBG("- E.\n");

	/*  */
	for (i = 0; i < Count; i++) {
		if (dbgWriteReg)
			LOG_DBG("module(%d), base(0x%lx),Addr(0x%lx), Val(0x%x)\n", module, (unsigned long)regBase,
			(unsigned long)(pReg[i].Addr), (unsigned int)(pReg[i].Val));

		if (((regBase + pReg[i].Addr) < (regBase + PAGE_SIZE)))
			DIP_WR32(regBase + pReg[i].Addr, pReg[i].Val);
		else
			LOG_ERR("wrong address(0x%lx)\n", (unsigned long)(regBase + pReg[i].Addr));

	}

	/*  */
	return Ret;
}



/*******************************************************************************
*
********************************************************************************/
static signed int DIP_WriteReg(struct DIP_REG_IO_STRUCT *pRegIo)
{
	signed int Ret = 0;
	/*    signed int TimeVd = 0;*/
	/*    signed int TimeExpdone = 0;*/
	/*    signed int TimeTasklet = 0;*/
	/* unsigned char* pData = NULL; */
	struct DIP_REG_STRUCT *pData = NULL;

	if (pRegIo->Count > 0xFFFFFFFF) {
		LOG_ERR("pRegIo->Count error");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	if (IspInfo.DebugMask & DIP_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n", (pRegIo->pData), (pRegIo->Count));

	/* pData = (unsigned char*)kmalloc((pRegIo->Count)*sizeof(DIP_REG_STRUCT), GFP_ATOMIC); */
	pData = kmalloc((pRegIo->Count) * sizeof(struct DIP_REG_STRUCT), GFP_ATOMIC);
	if (pData == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)\n",
		current->comm, current->pid, current->tgid);
		Ret = -ENOMEM;
		goto EXIT;
	}

	if ((void __user *)(pRegIo->pData) == NULL) {
		LOG_ERR("NULL pData");
		Ret = -EFAULT;
		goto EXIT;
	}

	/*  */
	if (copy_from_user(pData, (void __user *)(pRegIo->pData), pRegIo->Count * sizeof(struct DIP_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = DIP_WriteRegToHw(
		      pData,
		      pRegIo->Count);
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
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
static signed int dip_allocbuf(struct dip_imem_memory *pMemInfo)
{
	int ret = 0;
	struct ion_mm_data mm_data;
	struct ion_sys_data sys_data;
	struct ion_handle *handle = NULL;

	if (pMemInfo == NULL) {
		LOG_ERR("pMemInfo is NULL!!\n");
		ret = -ENOMEM;
		goto dip_allocbuf_exit;
	}

	if (dip_p2_ion_client == NULL) {
		LOG_ERR("dip_p2_ion_client is NULL!!\n");
		ret = -ENOMEM;
		goto dip_allocbuf_exit;
	}

	handle = ion_alloc(dip_p2_ion_client, pMemInfo->length, 0, ION_HEAP_MULTIMEDIA_MASK, 0);
	if (handle == NULL) {
		LOG_ERR("fail to alloc ion buffer, ret=%d\n", ret);
		ret = -ENOMEM;
		goto dip_allocbuf_exit;
	}
	pMemInfo->handle = (void *) handle;

	pMemInfo->va = (uintptr_t) ion_map_kernel(dip_p2_ion_client, handle);
	if (pMemInfo->va == 0) {
		LOG_ERR("fail to map va of buffer!\n");
		ret = -ENOMEM;
		goto dip_allocbuf_exit;
	}

	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER;
	mm_data.config_buffer_param.kernel_handle = handle;
	mm_data.config_buffer_param.module_id = 0;
	mm_data.config_buffer_param.security = 0;
	mm_data.config_buffer_param.coherent = 1;
	ret = ion_kernel_ioctl(dip_p2_ion_client, ION_CMD_MULTIMEDIA, (unsigned long)&mm_data);
	if (ret) {
		LOG_ERR("fail to config ion buffer, ret=%d\n", ret);
		ret = -ENOMEM;
		goto dip_allocbuf_exit;
	}

	sys_data.sys_cmd = ION_SYS_GET_PHYS;
	sys_data.get_phys_param.kernel_handle = handle;
	ret = ion_kernel_ioctl(dip_p2_ion_client, ION_CMD_SYSTEM, (unsigned long)&sys_data);
	pMemInfo->pa = sys_data.get_phys_param.phy_addr;

dip_allocbuf_exit:

	if (ret < 0) {
		if (handle)
			ion_free(dip_p2_ion_client, handle);
	}

	return ret;
}

/*******************************************************************************
*
********************************************************************************/
static void dip_freebuf(struct dip_imem_memory *pMemInfo)
{
	struct ion_handle *handle;

	if (pMemInfo == NULL) {
		LOG_ERR("pMemInfo is NULL!!\n");
		return;
	}

	handle = (struct ion_handle *) pMemInfo->handle;
	if (handle) {
		ion_unmap_kernel(dip_p2_ion_client, handle);
		ion_free(dip_p2_ion_client, handle);
	}

}
#endif

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_DumpBuffer(struct DIP_DUMP_BUFFER_STRUCT *pDumpBufStruct)
{
	signed int Ret = 0;

	if (pDumpBufStruct->BytesofBufferSize > 0xFFFFFFFF) {
		LOG_ERR("pDumpTuningBufStruct->BytesofBufferSize error");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	if ((void __user *)(pDumpBufStruct->pBuffer) == NULL) {
		LOG_ERR("NULL pDumpBufStruct->pBuffer");
		Ret = -EFAULT;
		goto EXIT;
	}
    /* Native Exception */
	switch (pDumpBufStruct->DumpCmd) {
	case DIP_DUMP_TPIPEBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize > MAX_DIP_TILE_TDR_HEX_NO) {
			LOG_ERR("tpipe size error");
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pTpipeBuffer == NULL)
				g_pTpipeBuffer = vmalloc(MAX_DIP_TILE_TDR_HEX_NO);
			else
				LOG_ERR("g_pTpipeBuffer:0x%pK is not NULL!!", g_pTpipeBuffer);
		}
		if (g_pTpipeBuffer != NULL) {
			if (copy_from_user(g_pTpipeBuffer, (void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
				LOG_ERR("copy_from_user g_pTpipeBuffer failed\n");
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("g_pTpipeBuffer kmalloc failed, g_bIonBufAllocated:%d\n", g_bIonBufferAllocated);
		}
#else
		if (copy_from_user(g_TpipeBuffer, (void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_TpipeBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p tpipebuf:0x%p is done!!\n", pDumpBufStruct->pBuffer, g_pTpipeBuffer);
		DumpBufferField = DumpBufferField | 0x1;
		break;
	case DIP_DUMP_TUNINGBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize > DIP_DIP_REG_SIZE) {
			LOG_ERR("tuning buf size error, size:0x%x", pDumpBufStruct->BytesofBufferSize);
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pTuningBuffer == NULL)
				g_pTuningBuffer = vmalloc(DIP_DIP_REG_SIZE);
			else
				LOG_ERR("g_TuningBuffer:0x%pK is not NULL!!", g_pTuningBuffer);
		}
		if (g_pTuningBuffer != NULL) {
			if (copy_from_user(g_pTuningBuffer, (void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
				LOG_ERR("copy_from_user g_pTuningBuffer failed\n");
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("ERROR: g_TuningBuffer kmalloc failed\n");
		}
#else
		if (copy_from_user(g_TuningBuffer, (void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_TuningBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p tuningbuf:0x%p is done!!\n", pDumpBufStruct->pBuffer, g_pTuningBuffer);
		DumpBufferField = DumpBufferField | 0x2;
		break;
	case DIP_DUMP_DIPVIRBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize > DIP_DIP_REG_SIZE) {
			LOG_ERR("vir dip buffer size error, size:0x%x", pDumpBufStruct->BytesofBufferSize);
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pVirDIPBuffer == NULL)
				g_pVirDIPBuffer = vmalloc(DIP_DIP_REG_SIZE);
			else
				LOG_ERR("g_pVirDIPBuffer:0x%pK is not NULL!!", g_pVirDIPBuffer);
		}
		if (g_pVirDIPBuffer != NULL) {
			if (copy_from_user(g_pVirDIPBuffer, (void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
				LOG_ERR("copy_from_user g_pVirDIPBuffer failed\n");
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("ERROR: g_pVirDIPBuffer kmalloc failed\n");
		}
#else
		if (copy_from_user(g_VirDIPBuffer, (void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_VirDIPBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p virdipbuf:0x%p is done!!\n", pDumpBufStruct->pBuffer, g_pVirDIPBuffer);
		DumpBufferField = DumpBufferField | 0x4;
		break;
	case DIP_DUMP_CMDQVIRBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize > MAX_DIP_CMDQ_BUFFER_SIZE) {
			LOG_ERR("cmdq buffer size error, size:0x%x", pDumpBufStruct->BytesofBufferSize);
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pCmdqBuffer == NULL)
				g_pCmdqBuffer = vmalloc(MAX_DIP_CMDQ_BUFFER_SIZE);
			else
				LOG_ERR("g_pCmdqBuffer:0x%pK is not NULL!!", g_pCmdqBuffer);
		}
		if (g_pCmdqBuffer != NULL) {
			if (copy_from_user(g_pCmdqBuffer, (void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
				LOG_ERR("copy_from_user g_pCmdqBuffer failed\n");
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("ERROR: g_pCmdqBuffer kmalloc failed\n");
		}
#else
		if (copy_from_user(g_CmdqBuffer, (void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_VirDIPBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p cmdqbuf:0x%p is done!!\n", pDumpBufStruct->pBuffer, g_pCmdqBuffer);
		DumpBufferField = DumpBufferField | 0x8;
		break;
	default:
		LOG_ERR("error dump buffer cmd:%d", pDumpBufStruct->DumpCmd);
		break;
	}
	if (g_bUserBufIsReady == MFALSE) {
		if ((DumpBufferField & 0xf) == 0xf) {
			g_bUserBufIsReady = MTRUE;
			DumpBufferField = 0;
			LOG_INF("DumpBufferField:0x%x, g_bUserBufIsReady:%d!!\n", DumpBufferField, g_bUserBufIsReady);
		}
	}
	/*  */
EXIT:

	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_SetMemInfo(struct DIP_MEM_INFO_STRUCT *pMemInfoStruct)
{
	signed int Ret = 0;
	/*  */
	if ((void __user *)(pMemInfoStruct->MemVa) == NULL) {
		LOG_ERR("NULL pMemInfoStruct->MemVa");
		Ret = -EFAULT;
		goto EXIT;
	}
	switch (pMemInfoStruct->MemInfoCmd) {
	case DIP_MEMORY_INFO_TPIPE_CMD:
		memcpy(&g_TpipeBaseAddrInfo, pMemInfoStruct, sizeof(struct DIP_MEM_INFO_STRUCT));
		LOG_INF("set tpipe memory info is done!!\n");
		break;
	case DIP_MEMORY_INFO_CMDQ_CMD:
		memcpy(&g_CmdqBaseAddrInfo, pMemInfoStruct, sizeof(struct DIP_MEM_INFO_STRUCT));
		LOG_INF("set comq memory info is done!!\n");
		break;
	default:
		LOG_ERR("error set memory info cmd:%d", pMemInfoStruct->MemInfoCmd);
		break;
	}
	/*  */
EXIT:

	return Ret;
}

/*******************************************************************************
*
********************************************************************************/

/*  */
/* isr dbg log , sw isr response counter , +1 when sw receive 1 sof isr. */
int DIP_Vsync_cnt[2] = {0, 0};

/*******************************************************************************
* update current idnex to working frame
********************************************************************************/
static signed int DIP_P2_BufQue_Update_ListCIdx(enum DIP_P2_BUFQUE_PROPERTY property,
enum DIP_P2_BUFQUE_LIST_TAG listTag)
{
	signed int ret = 0;
	signed int tmpIdx = 0;
	signed int cnt = 0;
	bool stop = false;
	int i = 0;
	enum DIP_P2_BUF_STATE_ENUM cIdxSts = DIP_P2_BUF_STATE_NONE;

	switch (listTag) {
	case DIP_P2_BUFQUE_LIST_TAG_UNIT:
		/* [1] check global pointer current sts */
		cIdxSts = P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].curr].bufSts;
		/* ////////////////////////////////////////////////////////////////////// */
		/* Assume we have the buffer list in the following situation */
		/* ++++++         ++++++         ++++++ */
		/* +  vss +         +  prv +         +  prv + */
		/* ++++++         ++++++         ++++++ */
		/* not deque         erased           enqued */
		/* done */
		/*  */
		/* if the vss deque is done, we should update the CurBufIdx to the next "enqued" */
		/* buffer node instead of just moving to the next buffer node */
		/* ////////////////////////////////////////////////////////////////////// */
		/* [2]traverse count needed */
		if (P2_FrameUnit_List_Idx[property].start <= P2_FrameUnit_List_Idx[property].end) {
			cnt = P2_FrameUnit_List_Idx[property].end - P2_FrameUnit_List_Idx[property].start;
		} else {
			cnt = _MAX_SUPPORT_P2_FRAME_NUM_ - P2_FrameUnit_List_Idx[property].start;
			cnt += P2_FrameUnit_List_Idx[property].end;
		}

		/* [3] update current index for frame unit list */
		tmpIdx = P2_FrameUnit_List_Idx[property].curr;
		switch (cIdxSts) {
		case DIP_P2_BUF_STATE_ENQUE:
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].curr].bufSts =
				DIP_P2_BUF_STATE_RUNNING;
			break;
		case DIP_P2_BUF_STATE_DEQUE_SUCCESS:
			do { /* to find the newest cur index */
				tmpIdx = (tmpIdx + 1) % _MAX_SUPPORT_P2_FRAME_NUM_;
				switch (P2_FrameUnit_List[property][tmpIdx].bufSts) {
				case DIP_P2_BUF_STATE_ENQUE:
				case DIP_P2_BUF_STATE_RUNNING:
					P2_FrameUnit_List[property][tmpIdx].bufSts = DIP_P2_BUF_STATE_RUNNING;
					P2_FrameUnit_List_Idx[property].curr = tmpIdx;
					stop = true;
					break;
				case DIP_P2_BUF_STATE_WAIT_DEQUE_FAIL:
				case DIP_P2_BUF_STATE_DEQUE_SUCCESS:
				case DIP_P2_BUF_STATE_DEQUE_FAIL:
				case DIP_P2_BUF_STATE_NONE:
				default:
					break;
				}
				i++;
			} while ((i < cnt) && (!stop));
			/* ////////////////////////////////////////////////////////////////////// */
			/* Assume we have the buffer list in the following situation */
			/* ++++++         ++++++         ++++++ */
			/* +  vss +         +  prv +         +  prv + */
			/* ++++++         ++++++         ++++++ */
			/* not deque         erased           erased */
			/* done */
			/*  */
			/* all the buffer node are deque done in the current moment, should update*/
			/* current index to the last node */
			/* if the vss deque is done, we should update the CurBufIdx to the last buffer node */
			/* ////////////////////////////////////////////////////////////////////// */
			if ((!stop) && (i == (cnt)))
				P2_FrameUnit_List_Idx[property].curr = P2_FrameUnit_List_Idx[property].end;

			break;
		case DIP_P2_BUF_STATE_WAIT_DEQUE_FAIL:
		case DIP_P2_BUF_STATE_DEQUE_FAIL:
			/* QQ. ADD ASSERT */
			break;
		case DIP_P2_BUF_STATE_NONE:
		case DIP_P2_BUF_STATE_RUNNING:
		default:
			break;
		}
		break;
	case DIP_P2_BUFQUE_LIST_TAG_PACKAGE:
	default:
		LOG_ERR("Wrong List tag(%d)\n", listTag);
		break;
	}
	return ret;
}
/*******************************************************************************
*
********************************************************************************/
static signed int DIP_P2_BufQue_Erase(enum DIP_P2_BUFQUE_PROPERTY property,
enum DIP_P2_BUFQUE_LIST_TAG listTag, signed int idx)
{
	signed int ret =  -1;
	bool stop = false;
	int i = 0;
	signed int cnt = 0;
	int tmpIdx = 0;

	switch (listTag) {
	case DIP_P2_BUFQUE_LIST_TAG_PACKAGE:
		tmpIdx = P2_FramePack_List_Idx[property].start;
		/* [1] clear buffer status */
		P2_FramePackage_List[property][idx].processID = 0x0;
		P2_FramePackage_List[property][idx].callerID = 0x0;
		P2_FramePackage_List[property][idx].dupCQIdx =  -1;
		P2_FramePackage_List[property][idx].frameNum = 0;
		P2_FramePackage_List[property][idx].dequedNum = 0;
		/* [2] update first index */
		if (P2_FramePackage_List[property][tmpIdx].dupCQIdx == -1) {
			/* traverse count needed, cuz user may erase the element but not the one */
			/* at first idx(pip or vss scenario) */
			if (P2_FramePack_List_Idx[property].start <= P2_FramePack_List_Idx[property].end) {
				cnt = P2_FramePack_List_Idx[property].end - P2_FramePack_List_Idx[property].start;
			} else {
				cnt = _MAX_SUPPORT_P2_PACKAGE_NUM_ - P2_FramePack_List_Idx[property].start;
				cnt += P2_FramePack_List_Idx[property].end;
			}
			do { /* to find the newest first lindex */
				tmpIdx = (tmpIdx + 1) % _MAX_SUPPORT_P2_PACKAGE_NUM_;
				switch (P2_FramePackage_List[property][tmpIdx].dupCQIdx) {
				case (-1):
					break;
				default:
					stop = true;
					P2_FramePack_List_Idx[property].start = tmpIdx;
					break;
				}
				i++;
			} while ((i < cnt) && (!stop));
			/* current last erased element in list is the one firstBufindex point at */
			/* and all the buffer node are deque done in the current moment, should */
			/* update first index to the last node */
			if ((!stop) && (i == cnt))
				P2_FramePack_List_Idx[property].start = P2_FramePack_List_Idx[property].end;

		}
		break;
	case DIP_P2_BUFQUE_LIST_TAG_UNIT:
		tmpIdx = P2_FrameUnit_List_Idx[property].start;
		/* [1] clear buffer status */
		P2_FrameUnit_List[property][idx].processID = 0x0;
		P2_FrameUnit_List[property][idx].callerID = 0x0;
		P2_FrameUnit_List[property][idx].cqMask =  0x0;
		P2_FrameUnit_List[property][idx].bufSts = DIP_P2_BUF_STATE_NONE;
		/* [2]update first index */
		if (P2_FrameUnit_List[property][tmpIdx].bufSts == DIP_P2_BUF_STATE_NONE) {
			/* traverse count needed, cuz user may erase the element but not the one at first idx */
			if (P2_FrameUnit_List_Idx[property].start <= P2_FrameUnit_List_Idx[property].end) {
				cnt = P2_FrameUnit_List_Idx[property].end - P2_FrameUnit_List_Idx[property].start;
			} else {
				cnt = _MAX_SUPPORT_P2_FRAME_NUM_ - P2_FrameUnit_List_Idx[property].start;
				cnt += P2_FrameUnit_List_Idx[property].end;
			}
			/* to find the newest first lindex */
			do {
				tmpIdx = (tmpIdx + 1) % _MAX_SUPPORT_P2_FRAME_NUM_;
				switch (P2_FrameUnit_List[property][tmpIdx].bufSts) {
				case DIP_P2_BUF_STATE_ENQUE:
				case DIP_P2_BUF_STATE_RUNNING:
				case DIP_P2_BUF_STATE_DEQUE_SUCCESS:
					stop = true;
					P2_FrameUnit_List_Idx[property].start = tmpIdx;
					break;
				case DIP_P2_BUF_STATE_WAIT_DEQUE_FAIL:
				case DIP_P2_BUF_STATE_DEQUE_FAIL:
					/* ASSERT */
					break;
				case DIP_P2_BUF_STATE_NONE:
				default:
					break;
				}
				i++;
			} while ((i < cnt) && (!stop));
			/* current last erased element in list is the one firstBufindex point at */
			/* and all the buffer node are deque done in the current moment, should */
			/* update first index to the last node */
			if ((!stop) && (i == (cnt)))
				P2_FrameUnit_List_Idx[property].start = P2_FrameUnit_List_Idx[property].end;

		}
		break;
	default:
		break;
	}
	return ret;
}

/*******************************************************************************
* get first matched element idnex
********************************************************************************/
static signed int DIP_P2_BufQue_GetMatchIdx(struct  DIP_P2_BUFQUE_STRUCT param,
		enum DIP_P2_BUFQUE_MATCH_TYPE matchType, enum DIP_P2_BUFQUE_LIST_TAG listTag)
{
	int idx = -1;
	int i = 0;
	int property;

	if (param.property >= DIP_P2_BUFQUE_PROPERTY_NUM) {
		LOG_ERR("property err(%d)\n", param.property);
		return idx;
	}
	property = param.property;

	switch (matchType) {
	case DIP_P2_BUFQUE_MATCH_TYPE_WAITDQ:
		/* traverse for finding the frame unit which had not beed dequeued of the process */
		if (P2_FrameUnit_List_Idx[property].start <= P2_FrameUnit_List_Idx[property].end) {
			for (i = P2_FrameUnit_List_Idx[property].start; i <= P2_FrameUnit_List_Idx[property].end; i++) {
				if ((P2_FrameUnit_List[property][i].processID == param.processID) &&
				    ((P2_FrameUnit_List[property][i].bufSts == DIP_P2_BUF_STATE_ENQUE) ||
				    (P2_FrameUnit_List[property][i].bufSts == DIP_P2_BUF_STATE_RUNNING))) {
					idx = i;
					break;
				}
			}
		} else {
			for (i = P2_FrameUnit_List_Idx[property].start; i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
				if ((P2_FrameUnit_List[property][i].processID == param.processID) &&
				    ((P2_FrameUnit_List[property][i].bufSts == DIP_P2_BUF_STATE_ENQUE) ||
				    (P2_FrameUnit_List[property][i].bufSts == DIP_P2_BUF_STATE_RUNNING))) {
					idx = i;
					break;
				}
			}
			if (idx !=  -1) {
				/*get in the first for loop*/
			} else {
				for (i = 0; i <= P2_FrameUnit_List_Idx[property].end; i++) {
					if ((P2_FrameUnit_List[property][i].processID == param.processID) &&
					    ((P2_FrameUnit_List[property][i].bufSts == DIP_P2_BUF_STATE_ENQUE) ||
					    (P2_FrameUnit_List[property][i].bufSts == DIP_P2_BUF_STATE_RUNNING))) {
						idx = i;
						break;
					}
				}
			}
		}
		break;
	case DIP_P2_BUFQUE_MATCH_TYPE_WAITFM:
		if (P2_FramePack_List_Idx[property].start <= P2_FramePack_List_Idx[property].end) {
			for (i = P2_FramePack_List_Idx[property].start; i <= P2_FramePack_List_Idx[property].end; i++) {
				if ((P2_FramePackage_List[property][i].processID == param.processID) &&
				(P2_FramePackage_List[property][i].callerID == param.callerID)) {
					idx = i;
					break;
				}
			}
		} else {
			for (i = P2_FramePack_List_Idx[property].start; i < _MAX_SUPPORT_P2_PACKAGE_NUM_; i++) {
				if ((P2_FramePackage_List[property][i].processID == param.processID) &&
				(P2_FramePackage_List[property][i].callerID == param.callerID)) {
					idx = i;
					break;
				}
			}
			if (idx !=  -1) {
				/*get in the first for loop*/
			} else {
				for (i = 0; i <= P2_FramePack_List_Idx[property].end; i++) {
					if ((P2_FramePackage_List[property][i].processID == param.processID) &&
					(P2_FramePackage_List[property][i].callerID == param.callerID)) {
						idx = i;
						break;
					}
				}
			}
		}
		break;
	case DIP_P2_BUFQUE_MATCH_TYPE_FRAMEOP:
		/* deque done notify */
		if (listTag == DIP_P2_BUFQUE_LIST_TAG_PACKAGE) {
			if (P2_FramePack_List_Idx[property].start <= P2_FramePack_List_Idx[property].end) {
				for (i = P2_FramePack_List_Idx[property].start;
				i <= P2_FramePack_List_Idx[property].end; i++) {
					if ((P2_FramePackage_List[property][i].processID == param.processID) &&
					(P2_FramePackage_List[property][i].callerID == param.callerID) &&
					    (P2_FramePackage_List[property][i].dupCQIdx == param.dupCQIdx) &&
					    (P2_FramePackage_List[property][i].dequedNum <
					    P2_FramePackage_List[property][i].frameNum)) {
						/* avoid race that dupCQ_1 of buffer2 enqued while dupCQ_1 */
						/* of buffer1 have beend deque done but not been erased yet */
						idx = i;
						break;
					}
				}
			} else {
				for (i = P2_FramePack_List_Idx[property].start; i < _MAX_SUPPORT_P2_PACKAGE_NUM_; i++) {
					if ((P2_FramePackage_List[property][i].processID == param.processID) &&
					(P2_FramePackage_List[property][i].callerID == param.callerID) &&
					    (P2_FramePackage_List[property][i].dupCQIdx == param.dupCQIdx) &&
					    (P2_FramePackage_List[property][i].dequedNum <
					    P2_FramePackage_List[property][i].frameNum)) {
						idx = i;
						break;
					}
				}
				if (idx !=  -1) {
					/*get in the first for loop*/
					break;
				}
				for (i = 0; i <= P2_FramePack_List_Idx[property].end; i++) {
					if ((P2_FramePackage_List[property][i].processID == param.processID) &&
					(P2_FramePackage_List[property][i].callerID == param.callerID) &&
						(P2_FramePackage_List[property][i].dupCQIdx == param.dupCQIdx) &&
						(P2_FramePackage_List[property][i].dequedNum <
						P2_FramePackage_List[property][i].frameNum)) {
						idx = i;
						break;
					}
				}
			}
		} else {
			if (P2_FrameUnit_List_Idx[property].start <= P2_FrameUnit_List_Idx[property].end) {
				for (i = P2_FrameUnit_List_Idx[property].start;
				i <= P2_FrameUnit_List_Idx[property].end; i++) {
					if ((P2_FrameUnit_List[property][i].processID == param.processID) &&
					(P2_FrameUnit_List[property][i].callerID == param.callerID)) {
						idx = i;
						break;
					}
				}
			} else {
				for (i = P2_FrameUnit_List_Idx[property].start; i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
					if ((P2_FrameUnit_List[property][i].processID == param.processID) &&
					(P2_FrameUnit_List[property][i].callerID == param.callerID)) {
						idx = i;
						break;
					}
				}
				if (idx !=  -1) {
					/*get in the first for loop*/
					break;
				}
				for (i = 0; i <= P2_FrameUnit_List_Idx[property].end; i++) {
					if ((P2_FrameUnit_List[property][i].processID == param.processID) &&
					(P2_FrameUnit_List[property][i].callerID == param.callerID)) {
						idx = i;
						break;
					}
				}
			}
		}
		break;
	default:
		break;
	}

	return idx;
}

/*******************************************************************************
*
********************************************************************************/
static inline unsigned int DIP_P2_BufQue_WaitEventState(struct DIP_P2_BUFQUE_STRUCT param,
		enum DIP_P2_BUFQUE_MATCH_TYPE type, signed int *idx)
{
	unsigned int ret = MFALSE;
	signed int index = -1;
	enum DIP_P2_BUFQUE_PROPERTY property;

	if (param.property >= DIP_P2_BUFQUE_PROPERTY_NUM) {
		LOG_ERR("property err(%d)\n", param.property);
		return ret;
	}
	property = param.property;
	/*  */
	switch (type) {
	case DIP_P2_BUFQUE_MATCH_TYPE_WAITDQ:
		spin_lock(&(SpinLock_P2FrameList));
		index = *idx;
		if (P2_FrameUnit_List[property][index].bufSts == DIP_P2_BUF_STATE_RUNNING)
			ret = MTRUE;

		spin_unlock(&(SpinLock_P2FrameList));
		break;
	case DIP_P2_BUFQUE_MATCH_TYPE_WAITFM:
		spin_lock(&(SpinLock_P2FrameList));
		index = *idx;
		if (P2_FramePackage_List[property][index].dequedNum == P2_FramePackage_List[property][index].frameNum)
			ret = MTRUE;

		spin_unlock(&(SpinLock_P2FrameList));
		break;
	case DIP_P2_BUFQUE_MATCH_TYPE_WAITFMEQD:
		spin_lock(&(SpinLock_P2FrameList));
		/* LOG_INF("check bf(%d_0x%x_0x%x/%d_%d)", param.property,
		* param.processID, param.callerID,index, *idx);
		*/
		index = DIP_P2_BufQue_GetMatchIdx(param, DIP_P2_BUFQUE_MATCH_TYPE_WAITFM,
				DIP_P2_BUFQUE_LIST_TAG_PACKAGE);
		if (index == -1) {
			/* LOG_INF("check bf(%d_0x%x_0x%x / %d_%d) ",param.property,
			* param.processID, param.callerID,index, *idx);
			*/
			spin_unlock(&(SpinLock_P2FrameList));
			ret = MFALSE;
		} else {
			*idx = index;
			/* LOG_INF("check bf(%d_0x%x_0x%x / %d_%d) ",param.property,
			* param.processID, param.callerID,index, *idx);
			*/
			spin_unlock(&(SpinLock_P2FrameList));
			ret = MTRUE;
		}
		break;
	default:
		break;
	}
	/*  */
	return ret;
}


/*******************************************************************************
*
********************************************************************************/
static signed int DIP_P2_BufQue_CTRL_FUNC(struct DIP_P2_BUFQUE_STRUCT param)
{
	signed int ret = 0;
	int i = 0, q = 0;
	int idx =  -1, idx2 =  -1;
	signed int restTime = 0;
	int property;

	if (param.property >= DIP_P2_BUFQUE_PROPERTY_NUM) {
		LOG_ERR("property err(%d)\n", param.property);
		ret = -EFAULT;
		return ret;
	}
	property = param.property;

	switch (param.ctrl) {
	case DIP_P2_BUFQUE_CTRL_ENQUE_FRAME:    /* signal that a specific buffer is enqueued */
		spin_lock(&(SpinLock_P2FrameList));
		/* (1) check the ring buffer list is full or not */
		if (((P2_FramePack_List_Idx[property].end + 1) % _MAX_SUPPORT_P2_PACKAGE_NUM_) ==
		P2_FramePack_List_Idx[property].start && (P2_FramePack_List_Idx[property].end !=  -1)) {
			LOG_ERR("pty(%d), F/L(%d_%d,%d), dCQ(%d,%d), RF/C/L(%d,%d,%d), sts(%d,%d,%d)",
			property, param.frameNum, P2_FramePack_List_Idx[property].start,
			P2_FramePack_List_Idx[property].end,
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].start].dupCQIdx,
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].dupCQIdx,
				P2_FrameUnit_List_Idx[property].start, P2_FrameUnit_List_Idx[property].curr,
				P2_FrameUnit_List_Idx[property].end,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].start].bufSts,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].curr].bufSts,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].end].bufSts);
			spin_unlock(&(SpinLock_P2FrameList));
			LOG_ERR("p2 frame package list is full, enque Fail.");
			ret =  -EFAULT;
			return ret;
		}
		{
			/*(2) add new to the last of the frame unit list */
			unsigned int cqmask = (param.dupCQIdx << 2) | (param.cQIdx << 1) | (param.burstQIdx);

			if (P2_FramePack_List_Idx[property].end < 0 || P2_FrameUnit_List_Idx[property].end < 0) {
#ifdef P2_DBG_LOG
				IRQ_LOG_KEEPER(DIP_IRQ_TYPE_INT_DIP_A_ST, 0, _LOG_DBG,
				"pty(%d) pD(0x%x_0x%x)MF/L(%d_%d %d) (%d %d) RF/C/L(%d %d %d) (%d %d %d) cqmsk(0x%x)\n",
				property, param.processID, param.callerID, param.frameNum,
				P2_FramePack_List_Idx[property].start, P2_FramePack_List_Idx[property].end,
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].start].dupCQIdx,
				P2_FramePackage_List[property][0].dupCQIdx,
				P2_FrameUnit_List_Idx[property].start, P2_FrameUnit_List_Idx[property].curr,
				P2_FrameUnit_List_Idx[property].end,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].start].bufSts,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].curr].bufSts,
				P2_FrameUnit_List[property][0].bufSts, cqmask);
#endif
			} else {
#ifdef P2_DBG_LOG
				IRQ_LOG_KEEPER(DIP_IRQ_TYPE_INT_DIP_A_ST, 0, _LOG_DBG,
				"pty(%d) pD(0x%x_0x%x) MF/L(%d_%d %d)(%d %d) RF/C/L(%d %d %d) (%d %d %d) cqmsk(0x%x)\n",
				property, param.processID, param.callerID, param.frameNum,
				P2_FramePack_List_Idx[property].start, P2_FramePack_List_Idx[property].end,
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].start].dupCQIdx,
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].dupCQIdx,
				P2_FrameUnit_List_Idx[property].start, P2_FrameUnit_List_Idx[property].curr,
				P2_FrameUnit_List_Idx[property].end,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].start].bufSts,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].curr].bufSts,
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].end].bufSts, cqmask);
#endif
			}
			if (P2_FrameUnit_List_Idx[property].start == P2_FrameUnit_List_Idx[property].end &&
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].start].bufSts ==
				DIP_P2_BUF_STATE_NONE) {
				/* frame unit list is empty */
				P2_FrameUnit_List_Idx[property].end =
					(P2_FrameUnit_List_Idx[property].end + 1) % _MAX_SUPPORT_P2_FRAME_NUM_;
				P2_FrameUnit_List_Idx[property].start = P2_FrameUnit_List_Idx[property].end;
				P2_FrameUnit_List_Idx[property].curr = P2_FrameUnit_List_Idx[property].end;
			} else if (P2_FrameUnit_List_Idx[property].curr == P2_FrameUnit_List_Idx[property].end &&
				P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].curr].bufSts ==
				DIP_P2_BUF_STATE_NONE) {
				/* frame unit list is not empty, but current/last is empty.
				* (all the enqueued frame is done but user have not called dequeue)
				*/
				P2_FrameUnit_List_Idx[property].end =
					(P2_FrameUnit_List_Idx[property].end + 1) % _MAX_SUPPORT_P2_FRAME_NUM_;
				P2_FrameUnit_List_Idx[property].curr = P2_FrameUnit_List_Idx[property].end;
			} else {
				P2_FrameUnit_List_Idx[property].end =
					(P2_FrameUnit_List_Idx[property].end + 1) % _MAX_SUPPORT_P2_FRAME_NUM_;
			}
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].end].processID = param.processID;
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].end].callerID = param.callerID;
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].end].cqMask = cqmask;
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[property].end].bufSts =
				DIP_P2_BUF_STATE_ENQUE;

			/* [3] add new frame package in list */
			if (param.burstQIdx == 0) {
				if (P2_FramePack_List_Idx[property].start == P2_FramePack_List_Idx[property].end &&
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].start].dupCQIdx == -1) {
					/* all managed buffer node is empty */
					P2_FramePack_List_Idx[property].end =
					(P2_FramePack_List_Idx[property].end + 1) % _MAX_SUPPORT_P2_PACKAGE_NUM_;
					P2_FramePack_List_Idx[property].start = P2_FramePack_List_Idx[property].end;
				} else {
					P2_FramePack_List_Idx[property].end =
					(P2_FramePack_List_Idx[property].end + 1) % _MAX_SUPPORT_P2_PACKAGE_NUM_;
				}
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].processID =
					param.processID;
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].callerID =
					param.callerID;
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].dupCQIdx =
					param.dupCQIdx;
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].frameNum =
					param.frameNum;
				P2_FramePackage_List[property][P2_FramePack_List_Idx[property].end].dequedNum = 0;
			}
		}
		/* [4]update global index */
		DIP_P2_BufQue_Update_ListCIdx(property, DIP_P2_BUFQUE_LIST_TAG_UNIT);
		spin_unlock(&(SpinLock_P2FrameList));
		IRQ_LOG_PRINTER(DIP_IRQ_TYPE_INT_DIP_A_ST, 0, _LOG_DBG);
		/* [5] wake up thread that wait for deque */
		wake_up_interruptible_all(&P2WaitQueueHead_WaitDeque);
		wake_up_interruptible_all(&P2WaitQueueHead_WaitFrameEQDforDQ);
		break;
	case DIP_P2_BUFQUE_CTRL_WAIT_DEQUE:    /* a dequeue thread is waiting to do dequeue */
		spin_lock(&(SpinLock_P2FrameList));
		idx = DIP_P2_BufQue_GetMatchIdx(param, DIP_P2_BUFQUE_MATCH_TYPE_WAITDQ, DIP_P2_BUFQUE_LIST_TAG_UNIT);
		spin_unlock(&(SpinLock_P2FrameList));
		if (idx ==  -1) {
			LOG_ERR("Do not find match buffer (pty/pid/cid: %d/0x%x/0x%x) to deque!",
			param.property, param.processID, param.callerID);
			ret =  -EFAULT;
			return ret;
		}
		{
			restTime = wait_event_interruptible_timeout(
					   P2WaitQueueHead_WaitDeque,
					   DIP_P2_BufQue_WaitEventState(param, DIP_P2_BUFQUE_MATCH_TYPE_WAITDQ, &idx),
					   DIP_UsToJiffies(15 * 1000000)); /* 15s */
			if (restTime == 0) {
				LOG_ERR("Wait Deque fail, idx(%d), pty(%d), pID(0x%x),cID(0x%x)",
				idx, param.property, param.processID, param.callerID);
				ret =  -EFAULT;
			} else if (restTime == -512) {
				LOG_ERR("be stopped, restime(%d)", restTime);
				ret =  -EFAULT;
				break;
			}
		}
		break;
	case DIP_P2_BUFQUE_CTRL_DEQUE_SUCCESS:             /* signal that a buffer is dequeued(success) */
		if (IspInfo.DebugMask & DIP_DBG_BUF_CTRL)
			LOG_DBG("dq cm(%d),pID(0x%x),cID(0x%x)\n", param.ctrl, param.processID, param.callerID);

		spin_lock(&(SpinLock_P2FrameList));
		/* [1]update buffer status for the current buffer */
		/* ////////////////////////////////////////////////////////////////////// */
		/* Assume we have the buffer list in the following situation */
		/* ++++++    ++++++ */
		/* +  vss +    +  prv + */
		/* ++++++    ++++++ */
		/*  */
		/* if the vss deque is not done(not blocking deque), dequeThread in userspace would change to */
		/* deque prv buffer(block deque) immediately to decrease ioctrl count. */
		/* -> vss buffer would be deque at next turn, so curBuf is still at vss buffer node */
		/* -> we should use param to find the current buffer index in Rlikst to update the buffer status */
		/*cuz deque success/fail may not be the first buffer in Rlist */
		/* ////////////////////////////////////////////////////////////////////// */
		idx2 = DIP_P2_BufQue_GetMatchIdx(param, DIP_P2_BUFQUE_MATCH_TYPE_FRAMEOP, DIP_P2_BUFQUE_LIST_TAG_UNIT);
		if (idx2 ==  -1) {
			spin_unlock(&(SpinLock_P2FrameList));
			LOG_ERR("ERRRRRRRRRRR findmatch index 2 fail (%d_0x%x_0x%x_%d, %d_%d)",
			param.property, param.processID, param.callerID, param.frameNum, param.cQIdx, param.dupCQIdx);
			ret =  -EFAULT;
			return ret;
		}
		if (param.ctrl == DIP_P2_BUFQUE_CTRL_DEQUE_SUCCESS)
			P2_FrameUnit_List[property][idx2].bufSts = DIP_P2_BUF_STATE_DEQUE_SUCCESS;
		else
			P2_FrameUnit_List[property][idx2].bufSts = DIP_P2_BUF_STATE_DEQUE_FAIL;

		/* [2]update dequeued num in managed buffer list */
		idx = DIP_P2_BufQue_GetMatchIdx(param, DIP_P2_BUFQUE_MATCH_TYPE_FRAMEOP,
		DIP_P2_BUFQUE_LIST_TAG_PACKAGE);
		if (idx ==  -1) {
			spin_unlock(&(SpinLock_P2FrameList));
			LOG_ERR("ERRRRRRRRRRR findmatch index 1 fail (%d_0x%x_0x%x_%d, %d_%d)",
			param.property, param.processID, param.callerID, param.frameNum, param.cQIdx, param.dupCQIdx);
			ret =  -EFAULT;
			return ret;
		}
		P2_FramePackage_List[property][idx].dequedNum++;
		/* [3]update global pointer */
		DIP_P2_BufQue_Update_ListCIdx(property, DIP_P2_BUFQUE_LIST_TAG_UNIT);
		/* [4]erase node in ring buffer list */
		DIP_P2_BufQue_Erase(property, DIP_P2_BUFQUE_LIST_TAG_UNIT, idx2);
		spin_unlock(&(SpinLock_P2FrameList));
		/* [5]wake up thread user that wait for a specific buffer and the thread that wait for deque */
		wake_up_interruptible_all(&P2WaitQueueHead_WaitFrame);
		wake_up_interruptible_all(&P2WaitQueueHead_WaitDeque);
		break;
	case DIP_P2_BUFQUE_CTRL_DEQUE_FAIL:                /* signal that a buffer is dequeued(fail) */
		/* QQ. ASSERT*/
		break;
	case DIP_P2_BUFQUE_CTRL_WAIT_FRAME:             /* wait for a specific buffer */
		/* [1]find first match buffer */
		/*LOG_INF("DIP_P2_BUFQUE_CTRL_WAIT_FRAME, before pty/pID/cID (%d/0x%x/0x%x),idx(%d)",
		*    property, param.processID, param.callerID, idx);
		*/
		/* wait for frame enqued due to user might call deque api before the frame is enqued to kernel */
		restTime = wait_event_interruptible_timeout(
				   P2WaitQueueHead_WaitFrameEQDforDQ,
				   DIP_P2_BufQue_WaitEventState(param, DIP_P2_BUFQUE_MATCH_TYPE_WAITFMEQD, &idx),
				   DIP_UsToJiffies(15 * 1000000)); /* wait 15s to get paired frame */
		if (restTime == 0) {
			LOG_ERR("could not find match buffer restTime(%d), pty/pID/cID (%d/0x%x/0x%x),idx(%d)",
			restTime, property, param.processID, param.callerID, idx);
			ret =  -EFAULT;
			return ret;
		} else if (restTime == -512) {
			LOG_ERR("be stopped, restime(%d)", restTime);
			ret =  -EFAULT;
			return ret;
		}

#ifdef P2_DBG_LOG
		LOG_INF("DIP_P2_BUFQUE_CTRL_WAIT_FRAME, after pty/pID/cID (%d/0x%x/0x%x),idx(%d)",
		property, param.processID, param.callerID, idx);
#endif
		spin_lock(&(SpinLock_P2FrameList));
		/* [2]check the buffer is dequeued or not */
		if (P2_FramePackage_List[property][idx].dequedNum == P2_FramePackage_List[property][idx].frameNum) {
			DIP_P2_BufQue_Erase(property, DIP_P2_BUFQUE_LIST_TAG_PACKAGE, idx);
			spin_unlock(&(SpinLock_P2FrameList));
			ret = 0;
#ifdef P2_DBG_LOG
			LOG_DBG("Frame is alreay dequeued, return user, pd(%d/0x%x/0x%x),idx(%d)",
			property, param.processID, param.callerID, idx);
#endif
			return ret;
		}
		{
			spin_unlock(&(SpinLock_P2FrameList));
			if (IspInfo.DebugMask & DIP_DBG_BUF_CTRL)
				LOG_DBG("=pd(%d/0x%x/0x%x_%d)wait(%d s)=\n", property,
					param.processID, param.callerID, idx, param.timeoutIns);

			/* [3]if not, goto wait event and wait for a signal to check */
			restTime = wait_event_interruptible_timeout(
					   P2WaitQueueHead_WaitFrame,
					   DIP_P2_BufQue_WaitEventState(param, DIP_P2_BUFQUE_MATCH_TYPE_WAITFM, &idx),
					   DIP_UsToJiffies(param.timeoutIns * 1000000));
			if (restTime == 0) {
				LOG_ERR("Dequeue Buffer fail, rT(%d),idx(%d) pty(%d), pID(0x%x),cID(0x%x)\n",
				restTime, idx, property, param.processID, param.callerID);
				ret =  -EFAULT;
				break;
			}
			if (restTime == -512) {
				LOG_ERR("be stopped, restime(%d)", restTime);
				ret =  -EFAULT;
				break;
			}
			{
				LOG_DBG("Dequeue Buffer ok, rT(%d),idx(%d) pty(%d), pID(0x%x),cID(0x%x)\n",
				restTime, idx, property, param.processID, param.callerID);
				spin_lock(&(SpinLock_P2FrameList));
				DIP_P2_BufQue_Erase(property, DIP_P2_BUFQUE_LIST_TAG_PACKAGE, idx);
				spin_unlock(&(SpinLock_P2FrameList));
			}
		}
		break;
	case DIP_P2_BUFQUE_CTRL_WAKE_WAITFRAME:   /* wake all slept users to check buffer is dequeued or not */
		wake_up_interruptible_all(&P2WaitQueueHead_WaitFrame);
		break;
	case DIP_P2_BUFQUE_CTRL_CLAER_ALL:           /* free all recored dequeued buffer */
		spin_lock(&(SpinLock_P2FrameList));
		for (q = 0; q < DIP_P2_BUFQUE_PROPERTY_NUM; q++) {
			for (i = 0; i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
				P2_FrameUnit_List[q][i].processID = 0x0;
				P2_FrameUnit_List[q][i].callerID = 0x0;
				P2_FrameUnit_List[q][i].cqMask = 0x0;
				P2_FrameUnit_List[q][i].bufSts = DIP_P2_BUF_STATE_NONE;
			}
			P2_FrameUnit_List_Idx[q].start = 0;
			P2_FrameUnit_List_Idx[q].curr = 0;
			P2_FrameUnit_List_Idx[q].end =  -1;
			/*  */
			for (i = 0; i < _MAX_SUPPORT_P2_PACKAGE_NUM_; i++) {
				P2_FramePackage_List[q][i].processID = 0x0;
				P2_FramePackage_List[q][i].callerID = 0x0;
				P2_FramePackage_List[q][i].dupCQIdx =  -1;
				P2_FramePackage_List[q][i].frameNum = 0;
				P2_FramePackage_List[q][i].dequedNum = 0;
			}
			P2_FramePack_List_Idx[q].start = 0;
			P2_FramePack_List_Idx[q].curr = 0;
			P2_FramePack_List_Idx[q].end =  -1;
		}
		spin_unlock(&(SpinLock_P2FrameList));
		break;
	default:
		LOG_ERR("do not support this ctrl cmd(%d)", param.ctrl);
		break;
	}
	return ret;
}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_REGISTER_IRQ_USERKEY(char *userName)
{
	int key =  -1;
	int i = 0;

	spin_lock((spinlock_t *)(&SpinLock_UserKey));

	/* 1. check the current users is full or not */
	if (FirstUnusedIrqUserKey == IRQ_USER_NUM_MAX) {
		key = -1;
	} else {
		/* 2. check the user had registered or not */
		for (i = 1; i < FirstUnusedIrqUserKey; i++) {
			/* index 0 is for all the users that do not register irq first */
			if (strcmp((void *)IrqUserKey_UserInfo[i].userName, userName) == 0) {
				key = IrqUserKey_UserInfo[i].userKey;
				break;
			}
		}

		/* 3.return new userkey for user if the user had not registered before */
		if (key < 0) {
			/* IrqUserKey_UserInfo[i].userName=userName; */
			memset((void *)IrqUserKey_UserInfo[i].userName, 0, sizeof(IrqUserKey_UserInfo[i].userName));
			strncpy((void *)IrqUserKey_UserInfo[i].userName, userName, USERKEY_STR_LEN-1);
			IrqUserKey_UserInfo[i].userKey = FirstUnusedIrqUserKey;
			key = FirstUnusedIrqUserKey;
			FirstUnusedIrqUserKey++;
		}
	}

	spin_unlock((spinlock_t *)(&SpinLock_UserKey));
	LOG_INF("User(%s)key(%d)\n", userName, key);
	return key;
}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_FLUSH_IRQ(struct DIP_WAIT_IRQ_STRUCT *irqinfo)
{
	unsigned long flags; /* old: unsigned int flags;*//* FIX to avoid build warning */

	LOG_INF("type(%d)userKey(%d)St(0x%x)",
		irqinfo->Type, irqinfo->EventInfo.UserKey, irqinfo->EventInfo.Status);

	if (irqinfo->Type >= DIP_IRQ_TYPE_AMOUNT) {
		LOG_ERR("FLUSH_IRQ: type error(%d)", irqinfo->Type);
		return -EFAULT;
	}

	if (irqinfo->EventInfo.UserKey >= IRQ_USER_NUM_MAX || irqinfo->EventInfo.UserKey < 0) {
		LOG_ERR("FLUSH_IRQ: userkey error(%d)", irqinfo->EventInfo.UserKey);
		return -EFAULT;
	}

	/* 1. enable signal */
	spin_lock_irqsave(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);
	IspInfo.IrqInfo.Status[irqinfo->Type][irqinfo->EventInfo.UserKey] |=
	irqinfo->EventInfo.Status;
	spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);

	/* 2. force to wake up the user that are waiting for that signal */
	wake_up_interruptible(&IspInfo.WaitQueueHead[irqinfo->Type]);

	return 0;
}


/*******************************************************************************
*
********************************************************************************/
static signed int DIP_WaitIrq(struct DIP_WAIT_IRQ_STRUCT *WaitIrq)
{

	signed int Ret = 0;

	return Ret;
}


#ifdef ENABLE_KEEP_ION_HANDLE
/*******************************************************************************
*
********************************************************************************/
static void DIP_ion_init(void)
{
	if (!pIon_client && g_ion_device)
		pIon_client = ion_client_create(g_ion_device, "camera_dip");

	if (!pIon_client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}

	if (IspInfo.DebugMask & DIP_DBG_ION_CTRL)
		LOG_INF("create ion client 0x%p\n", pIon_client);
}

/*******************************************************************************
*
********************************************************************************/
static void DIP_ion_uninit(void)
{
	if (!pIon_client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}

	if (IspInfo.DebugMask & DIP_DBG_ION_CTRL)
		LOG_INF("destroy ion client 0x%p\n", pIon_client);

	ion_client_destroy(pIon_client);

	pIon_client = NULL;
}

/*******************************************************************************
*
********************************************************************************/
static struct ion_handle *DIP_ion_import_handle(struct ion_client *client, int fd)
{
	struct ion_handle *handle = NULL;

	if (!client) {
		LOG_ERR("invalid ion client!\n");
		return handle;
	}
	if (fd == -1) {
		LOG_ERR("invalid ion fd!\n");
		return handle;
	}

	handle = ion_import_dma_buf(client, fd);

	if (IS_ERR(handle)) {
		LOG_ERR("import ion handle failed!\n");
		return NULL;
	}

	if (IspInfo.DebugMask & DIP_DBG_ION_CTRL)
		LOG_INF("[ion_import_hd] Hd(0x%p)\n", handle);
	return handle;
}

/*******************************************************************************
*
********************************************************************************/
static void DIP_ion_free_handle(struct ion_client *client, struct ion_handle *handle)
{
	if (!client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}
	if (!handle)
		return;

	if (IspInfo.DebugMask & DIP_DBG_ION_CTRL)
		LOG_INF("[ion_free_hd] Hd(0x%p)\n", handle);

	ion_free(client, handle);

}

/*******************************************************************************
*
********************************************************************************/
static void DIP_ion_free_handle_by_module(unsigned int module)
{
	int i, j;
	signed int nFd;
	struct ion_handle *p_IonHnd;
	struct T_ION_TBL *ptbl = &gION_TBL[module];

	if (IspInfo.DebugMask & DIP_DBG_ION_CTRL)
		LOG_INF("[ion_free_hd_by_module]%d\n", module);

	for (i = 0; i < _dma_max_wr_; i++) {
		unsigned int jump = i*_ion_keep_max_;

		for (j = 0; j < _ion_keep_max_ ; j++) {
			spin_lock(&(ptbl->pLock[i]));
			/* */
			if (ptbl->pIonFd[jump + j] == 0) {
				spin_unlock(&(ptbl->pLock[i]));
				continue;
			}
			nFd = ptbl->pIonFd[jump + j];
			p_IonHnd = ptbl->pIonHnd[jump + j];
			/* */
			ptbl->pIonFd[jump + j] = 0;
			ptbl->pIonHnd[jump + j] = NULL;
			ptbl->pIonCt[jump + j] = 0;
			spin_unlock(&(ptbl->pLock[i]));
			/* */
			if (IspInfo.DebugMask & DIP_DBG_ION_CTRL) {
				LOG_INF("ion_free: dev(%d)dma(%d)j(%d)fd(%d)Hnd(0x%p)\n",
					module, i, j, nFd, p_IonHnd);
			}
			DIP_ion_free_handle(pIon_client, p_IonHnd);/*can't in spin_lock*/
		}
	}
}

#endif


/*******************************************************************************
*
********************************************************************************/
static long DIP_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	signed int Ret = 0;
	/*  */
	/*    bool   HoldEnable = MFALSE;*/
	unsigned int DebugFlag[3] = {0};
	/*    unsigned int pid = 0;*/
	struct DIP_REG_IO_STRUCT       RegIo;
	struct DIP_DUMP_BUFFER_STRUCT DumpBufStruct;
	struct DIP_MEM_INFO_STRUCT MemInfoStruct;
	struct DIP_WAIT_IRQ_STRUCT     IrqInfo;
	struct DIP_CLEAR_IRQ_STRUCT    ClearIrq;
	struct DIP_USER_INFO_STRUCT *pUserInfo;
	struct  DIP_P2_BUFQUE_STRUCT    p2QueBuf;
	unsigned int                 wakelock_ctrl;
	unsigned int                 module;
	unsigned long flags; /* old: unsigned int flags;*//* FIX to avoid build warning */
	int userKey =  -1;
	struct DIP_REGISTER_USERKEY_STRUCT RegUserKey;
	int i;
	#ifdef ENABLE_KEEP_ION_HANDLE
	struct DIP_DEV_ION_NODE_STRUCT IonNode;
	struct ion_handle *handle;
	struct ion_handle *p_IonHnd;
	#endif

	/*  */
	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(process, pid, tgid)=(%s, %d, %d)\n",
			current->comm, current->pid, current->tgid);
		return -EFAULT;
	}
	/*  */
	pUserInfo = (struct DIP_USER_INFO_STRUCT *)(pFile->private_data);
	/*  */
	switch (Cmd) {
	case DIP_WAKELOCK_CTRL:
		if (copy_from_user(&wakelock_ctrl, (void *)Param, sizeof(unsigned int)) != 0) {
			LOG_ERR("get DIP_WAKELOCK_CTRL from user fail\n");
			Ret = -EFAULT;
		} else {
			if (wakelock_ctrl == 1) {       /* Enable     wakelock */
				if (g_bWaitLock == 0) {
#ifdef CONFIG_PM_WAKELOCKS
					__pm_stay_awake(&dip_wake_lock);
#else
					wake_lock(&dip_wake_lock);
#endif
					g_bWaitLock = 1;
					LOG_DBG("wakelock enable!!\n");
				}
			} else {        /* Disable wakelock */
				if (g_bWaitLock == 1) {
#ifdef CONFIG_PM_WAKELOCKS
					__pm_relax(&dip_wake_lock);
#else
					wake_unlock(&dip_wake_lock);
#endif
					g_bWaitLock = 0;
					LOG_DBG("wakelock disable!!\n");
				}
			}
		}
		break;
	case DIP_RESET_BY_HWMODULE: {
		if (copy_from_user(&module, (void *)Param, sizeof(unsigned int)) != 0) {
			LOG_ERR("get hwmodule from user fail\n");
			Ret = -EFAULT;
		} else {
			DIP_Reset(module);
		}
		break;
	}
	case DIP_READ_REGISTER: {
		if (copy_from_user(&RegIo, (void *)Param, sizeof(struct DIP_REG_IO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented in DIP_ReadReg(...) */
			Ret = DIP_ReadReg(&RegIo);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case DIP_WRITE_REGISTER: {
		if (copy_from_user(&RegIo, (void *)Param, sizeof(struct DIP_REG_IO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented in DIP_WriteReg(...) */
			Ret = DIP_WriteReg(&RegIo);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case DIP_WAIT_IRQ: {
		if (copy_from_user(&IrqInfo, (void *)Param, sizeof(struct DIP_WAIT_IRQ_STRUCT)) == 0) {
			/*  */
			if ((IrqInfo.Type >= DIP_IRQ_TYPE_AMOUNT) || (IrqInfo.Type < 0)) {
				Ret = -EFAULT;
				LOG_ERR("invalid type(%d)\n", IrqInfo.Type);
				goto EXIT;
			}

			if ((IrqInfo.EventInfo.UserKey >= IRQ_USER_NUM_MAX) || (IrqInfo.EventInfo.UserKey < 0)) {
				LOG_ERR("invalid userKey(%d), max(%d), force userkey = 0\n",
					IrqInfo.EventInfo.UserKey, IRQ_USER_NUM_MAX);
				IrqInfo.EventInfo.UserKey = 0;
			}
#ifdef ENABLE_WAITIRQ_LOG
			LOG_INF("IRQ type(%d), userKey(%d), timeout(%d), userkey(%d), status(%d)\n",
				IrqInfo.Type, IrqInfo.EventInfo.UserKey, IrqInfo.EventInfo.Timeout,
				IrqInfo.EventInfo.UserKey, IrqInfo.EventInfo.Status);
#endif
			Ret = DIP_WaitIrq(&IrqInfo);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case DIP_CLEAR_IRQ: {
		if (copy_from_user(&ClearIrq, (void *)Param, sizeof(struct DIP_CLEAR_IRQ_STRUCT)) == 0) {
			LOG_DBG("DIP_CLEAR_IRQ Type(%d)\n", ClearIrq.Type);

			if ((ClearIrq.Type >= DIP_IRQ_TYPE_AMOUNT) || (ClearIrq.Type < 0)) {
				Ret = -EFAULT;
				LOG_ERR("invalid type(%d)\n", ClearIrq.Type);
				goto EXIT;
			}

			/*  */
			if ((ClearIrq.EventInfo.UserKey >= IRQ_USER_NUM_MAX) || (ClearIrq.EventInfo.UserKey < 0)) {
				LOG_ERR("errUserEnum(%d)", ClearIrq.EventInfo.UserKey);
				Ret = -EFAULT;
				goto EXIT;
			}

			i = ClearIrq.EventInfo.UserKey;/*avoid line over 120 char per line */
			LOG_DBG("DIP_CLEAR_IRQ:Type(%d),Status(0x%x),IrqStatus(0x%x)\n",
				ClearIrq.Type, ClearIrq.EventInfo.Status,
				IspInfo.IrqInfo.Status[ClearIrq.Type][i]);
			spin_lock_irqsave(&(IspInfo.SpinLockIrq[ClearIrq.Type]), flags);
			IspInfo.IrqInfo.Status[ClearIrq.Type][ClearIrq.EventInfo.UserKey] &=
				(~ClearIrq.EventInfo.Status);
			spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[ClearIrq.Type]), flags);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	/*  */
	case DIP_REGISTER_IRQ_USER_KEY:
		if (copy_from_user(&RegUserKey, (void *)Param, sizeof(struct DIP_REGISTER_USERKEY_STRUCT)) == 0) {
			userKey = DIP_REGISTER_IRQ_USERKEY(RegUserKey.userName);
			RegUserKey.userKey = userKey;
			if (copy_to_user((void *)Param, &RegUserKey, sizeof(struct DIP_REGISTER_USERKEY_STRUCT)) != 0)
				LOG_ERR("copy_to_user failed\n");

			if (RegUserKey.userKey < 0) {
				LOG_ERR("query irq user key fail\n");
				Ret = -1;
			}
		} else {
			LOG_ERR("copy from user fail\n");
		}

		break;
	/*  */
	case DIP_FLUSH_IRQ_REQUEST:
		if (copy_from_user(&IrqInfo, (void *)Param, sizeof(struct DIP_WAIT_IRQ_STRUCT)) == 0) {
			if ((IrqInfo.EventInfo.UserKey >= IRQ_USER_NUM_MAX) || (IrqInfo.EventInfo.UserKey < 0)) {
				LOG_ERR("invalid userKey(%d), max(%d)\n", IrqInfo.EventInfo.UserKey, IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((IrqInfo.Type >= DIP_IRQ_TYPE_AMOUNT) || (IrqInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)\n", IrqInfo.Type, DIP_IRQ_TYPE_AMOUNT);
				Ret = -EFAULT;
				break;
			}

			Ret = DIP_FLUSH_IRQ(&IrqInfo);
		}
		break;
	/*  */
	case DIP_P2_BUFQUE_CTRL:
		if (copy_from_user(&p2QueBuf, (void *)Param, sizeof(struct DIP_P2_BUFQUE_STRUCT)) == 0) {
			p2QueBuf.processID = pUserInfo->Pid;
			Ret = DIP_P2_BufQue_CTRL_FUNC(p2QueBuf);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;

	case DIP_DEBUG_FLAG:
		if (copy_from_user(DebugFlag, (void *)Param, sizeof(unsigned int)) == 0) {

			IspInfo.DebugMask = DebugFlag[0];

			/* LOG_DBG("FBC kernel debug level = %x\n",IspInfo.DebugMask); */
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;

		break;
	#ifdef ENABLE_KEEP_ION_HANDLE
	case DIP_ION_IMPORT:
		if (copy_from_user(&IonNode, (void *)Param, sizeof(struct DIP_DEV_ION_NODE_STRUCT)) == 0) {
			struct T_ION_TBL *ptbl = NULL;
			unsigned int jump;

			if (!pIon_client) {
				LOG_ERR("ion_import: invalid ion client!\n");
				Ret = -EFAULT;
				break;
			}

			if (IonNode.devNode >= DIP_DEV_NODE_NUM) {
				LOG_ERR("[DIP_ION_IMPORT]devNode should be smaller than DIP_DEV_NODE_NUM");
				Ret = -EFAULT;
				break;
			}

			ptbl = &gION_TBL[IonNode.devNode];
			if (ptbl->node != IonNode.devNode) {
				LOG_ERR("ion_import: devNode not support(%d)!\n", IonNode.devNode);
				Ret = -EFAULT;
				break;
			}
			if (IonNode.dmaPort < 0 || IonNode.dmaPort >= _dma_max_wr_) {
				LOG_ERR("ion_import: dmaport error:%d(0~%d)\n", IonNode.dmaPort, _dma_max_wr_);
				Ret = -EFAULT;
				break;
			}
			jump = IonNode.dmaPort*_ion_keep_max_;
			if (IonNode.memID <= 0) {
				LOG_ERR("ion_import: dma(%d)invalid ion fd(%d)\n", IonNode.dmaPort, IonNode.memID);
				Ret = -EFAULT;
				break;
			}
			spin_lock(&(ptbl->pLock[IonNode.dmaPort]));
			/* */
			/* check if memID is exist */
			for (i = 0; i < _ion_keep_max_; i++) {
				if (ptbl->pIonFd[jump + i] == IonNode.memID)
					break;
			}
			spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
			/* */
			if (i < _ion_keep_max_) {
				if (IspInfo.DebugMask & DIP_DBG_ION_CTRL) {
					LOG_INF("ion_import: already exist: dev(%d)dma(%d)i(%d)fd(%d)Hnd(0x%p)\n",
						IonNode.devNode, IonNode.dmaPort, i, IonNode.memID,
						ptbl->pIonHnd[jump + i]);
				}
				/* User might allocate a big memory and divid it into many buffers,
				*	the ion FD of these buffers is the same,
				*	so we must check there has no users take this memory
				*/
				ptbl->pIonCt[jump + i]++;

				break;
			}
			/* */
			handle = DIP_ion_import_handle(pIon_client, IonNode.memID);
			if (!handle) {
				Ret = -EFAULT;
				break;
			}
			/* */
			spin_lock(&(ptbl->pLock[IonNode.dmaPort]));
			for (i = 0; i < _ion_keep_max_; i++) {
				if (ptbl->pIonFd[jump + i] == 0) {
					ptbl->pIonFd[jump + i] = IonNode.memID;
					ptbl->pIonHnd[jump + i] = handle;

					/* User might allocate a big memory and divid it into many buffers,
					* the ion FD of these buffers is the same,
					* so we must check there has no users take this memory
					*/
					ptbl->pIonCt[jump + i]++;

					if (IspInfo.DebugMask & DIP_DBG_ION_CTRL) {
						LOG_INF("ion_import: dev(%d)dma(%d)i(%d)fd(%d)Hnd(0x%p)\n",
							IonNode.devNode, IonNode.dmaPort, i,
							IonNode.memID, handle);
					}
					break;
				}
			}
			spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
			/* */
			if (i == _ion_keep_max_) {
				LOG_ERR("ion_import: dma(%d)no empty space in list(%d_%d)\n",
					IonNode.dmaPort, IonNode.memID, _ion_keep_max_);
				DIP_ion_free_handle(pIon_client, handle);/*can't in spin_lock*/
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("[ion import]copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case DIP_ION_FREE:
		if (copy_from_user(&IonNode, (void *)Param, sizeof(struct DIP_DEV_ION_NODE_STRUCT)) == 0) {
			struct T_ION_TBL *ptbl = NULL;
			unsigned int jump;

			if (!pIon_client) {
				LOG_ERR("ion_free: invalid ion client!\n");
				Ret = -EFAULT;
				break;
			}

			if (IonNode.devNode >= DIP_DEV_NODE_NUM) {
				LOG_ERR("[DIP_ION_FREE]devNode should be smaller than DIP_DEV_NODE_NUM");
				Ret = -EFAULT;
				break;
			}

			ptbl = &gION_TBL[IonNode.devNode];
			if (ptbl->node != IonNode.devNode) {
				LOG_ERR("ion_free: devNode not support(%d)!\n", IonNode.devNode);
				Ret = -EFAULT;
				break;
			}
			if (IonNode.dmaPort < 0 || IonNode.dmaPort >= _dma_max_wr_) {
				LOG_ERR("ion_free: dmaport error:%d(0~%d)\n", IonNode.dmaPort, _dma_max_wr_);
				Ret = -EFAULT;
				break;
			}
			jump = IonNode.dmaPort*_ion_keep_max_;
			if (IonNode.memID <= 0) {
				LOG_ERR("ion_free: invalid ion fd(%d)\n", IonNode.memID);
				Ret = -EFAULT;
				break;
			}

			/* check if memID is exist */
			spin_lock(&(ptbl->pLock[IonNode.dmaPort]));
			for (i = 0; i < _ion_keep_max_; i++) {
				if (ptbl->pIonFd[jump + i] == IonNode.memID)
					break;
			}
			if (i == _ion_keep_max_) {
				spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
				LOG_ERR("ion_free: can't find ion dev(%d)dma(%d)fd(%d) in list\n",
					IonNode.devNode, IonNode.dmaPort, IonNode.memID);
				Ret = -EFAULT;

				break;
			}
			/* User might allocate a big memory and divid it into many buffers,
			*	the ion FD of these buffers is the same,
			*	so we must check there has no users take this memory
			*/
			if (--ptbl->pIonCt[jump + i] > 0) {
				spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
				if (IspInfo.DebugMask & DIP_DBG_ION_CTRL) {
					LOG_INF("ion_free: user ct(%d): dev(%d)dma(%d)i(%d)fd(%d)\n",
						ptbl->pIonCt[jump + i],
						IonNode.devNode, IonNode.dmaPort, i,
						IonNode.memID);
				}
				break;
			} else if (ptbl->pIonCt[jump + i] < 0) {
				spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
				LOG_ERR("ion_free: free more than import (%d): dev(%d)dma(%d)i(%d)fd(%d)\n",
						ptbl->pIonCt[jump + i],
						IonNode.devNode, IonNode.dmaPort, i,
						IonNode.memID);
				Ret = -EFAULT;
				break;
			}

			if (IspInfo.DebugMask & DIP_DBG_ION_CTRL) {
				LOG_INF("ion_free: dev(%d)dma(%d)i(%d)fd(%d)Hnd(0x%p)Ct(%d)\n",
					IonNode.devNode, IonNode.dmaPort, i,
					IonNode.memID,
					ptbl->pIonHnd[jump + i],
					ptbl->pIonCt[jump + i]);
			}

			p_IonHnd = ptbl->pIonHnd[jump + i];
			ptbl->pIonFd[jump + i] = 0;
			ptbl->pIonHnd[jump + i] = NULL;
			spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
			/* */
			DIP_ion_free_handle(pIon_client, p_IonHnd);/*can't in spin_lock*/
		} else {
			LOG_ERR("[ion free]copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case DIP_ION_FREE_BY_HWMODULE:
		if (copy_from_user(&module, (void *)Param, sizeof(unsigned int)) == 0) {
			if (module >= DIP_DEV_NODE_NUM) {
				LOG_ERR("[DIP_ION_FREE_BY_HWMODULE]module should be smaller than DIP_DEV_NODE_NUM");
				Ret = -EFAULT;
				break;
			}
			if (gION_TBL[module].node != module) {
				LOG_ERR("module error(%d)\n", module);
				Ret = -EFAULT;
				break;
			}

			DIP_ion_free_handle_by_module(module);
		} else {
			LOG_ERR("[ion free by module]copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	#endif
	case DIP_GET_DUMP_INFO: {
		if (copy_to_user((void *)Param, &g_dumpInfo, sizeof(struct DIP_GET_DUMP_INFO_STRUCT)) != 0) {
			LOG_ERR("DIP_GET_DUMP_INFO copy to user fail");
			Ret = -EFAULT;
		}
		break;
	}
	case DIP_DUMP_BUFFER: {
		if (copy_from_user(&DumpBufStruct, (void *)Param, sizeof(struct DIP_DUMP_BUFFER_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented in DIP_DumpTuningBuffer(...) */
			Ret = DIP_DumpBuffer(&DumpBufStruct);
		} else {
			LOG_ERR("DIP_DUMP_TUNING_BUFFER copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case DIP_SET_MEM_INFO: {
		if (copy_from_user(&MemInfoStruct, (void *)Param, sizeof(struct DIP_MEM_INFO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented in DIP_SetMemInfo(...) */
			Ret = DIP_SetMemInfo(&MemInfoStruct);
		} else {
			LOG_ERR("DIP_SET_MEM_INFO copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	default:
	{
		LOG_ERR("Unknown Cmd(%d)\n", Cmd);
		Ret = -EPERM;
		break;
	}
	}
	/*  */
EXIT:
	if (Ret != 0)
		LOG_ERR("Fail, Cmd(%d), Pid(%d), (process, pid, tgid)=(%s, %d, %d)\n",
			Cmd, pUserInfo->Pid, current->comm, current->pid, current->tgid);
	/*  */
	return Ret;
}

#ifdef CONFIG_COMPAT

/*******************************************************************************
*
********************************************************************************/
static int compat_get_dip_read_register_data(
	struct compat_DIP_REG_IO_STRUCT __user *data32,
	struct DIP_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->pData);
	err |= put_user(compat_ptr(uptr), &data->pData);
	err |= get_user(count, &data32->Count);
	err |= put_user(count, &data->Count);
	return err;
}

static int compat_put_dip_read_register_data(
	struct compat_DIP_REG_IO_STRUCT __user *data32,
	struct DIP_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	/*      compat_uptr_t uptr;*/
	int err = 0;
	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr),     &data->pData); */
	/* err |= put_user(uptr, &data32->pData); */
	err |= get_user(count, &data->Count);
	err |= put_user(count, &data32->Count);
	return err;
}

static int compat_get_dip_dump_buffer(
	struct compat_DIP_DUMP_BUFFER_STRUCT __user *data32,
	struct DIP_DUMP_BUFFER_STRUCT __user *data)
{
	compat_uint_t count;
	compat_uint_t cmd;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(cmd, &data32->DumpCmd);
	err |= put_user(cmd, &data->DumpCmd);
	err |= get_user(uptr, &data32->pBuffer);
	err |= put_user(compat_ptr(uptr), &data->pBuffer);
	err |= get_user(count, &data32->BytesofBufferSize);
	err |= put_user(count, &data->BytesofBufferSize);
	return err;
}

static int compat_get_dip_mem_info(
	struct compat_DIP_MEM_INFO_STRUCT __user *data32,
	struct DIP_MEM_INFO_STRUCT __user *data)
{
	compat_uint_t cmd;
	compat_uint_t mempa;
	compat_uptr_t uptr;
	compat_uint_t size;
	int err = 0;

	err = get_user(cmd, &data32->MemInfoCmd);
	err |= put_user(cmd, &data->MemInfoCmd);
	err |= get_user(mempa, &data32->MemPa);
	err |= put_user(mempa, &data->MemPa);
	err |= get_user(uptr, &data32->MemVa);
	err |= put_user(compat_ptr(uptr), &data->MemVa);
	err |= get_user(size, &data32->MemSizeDiff);
	err |= put_user(size, &data->MemSizeDiff);
	return err;
}

static long DIP_ioctl_compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_DIP_READ_REGISTER: {
		struct compat_DIP_REG_IO_STRUCT __user *data32;
		struct DIP_REG_IO_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_dip_read_register_data(data32, data);
		if (err) {
			LOG_INF("compat_get_dip_read_register_data error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, DIP_READ_REGISTER, (unsigned long)data);
		err = compat_put_dip_read_register_data(data32, data);
		if (err) {
			LOG_INF("compat_put_dip_read_register_data error!!!\n");
			return err;
		}
		return ret;
	}
	case COMPAT_DIP_WRITE_REGISTER: {
		struct compat_DIP_REG_IO_STRUCT __user *data32;
		struct DIP_REG_IO_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_dip_read_register_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_DIP_WRITE_REGISTER error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, DIP_WRITE_REGISTER, (unsigned long)data);
		return ret;
	}

	case COMPAT_DIP_DEBUG_FLAG: {
		/* compat_ptr(arg) will convert the arg */
		ret = filp->f_op->unlocked_ioctl(filp, DIP_DEBUG_FLAG, (unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_DIP_WAKELOCK_CTRL: {
		ret =
			filp->f_op->unlocked_ioctl(filp, DIP_WAKELOCK_CTRL,
						   (unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_DIP_RESET_BY_HWMODULE: {
		ret =
			filp->f_op->unlocked_ioctl(filp, DIP_RESET_BY_HWMODULE,
						   (unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_DIP_DUMP_BUFFER: {
		struct compat_DIP_DUMP_BUFFER_STRUCT __user *data32;
		struct DIP_DUMP_BUFFER_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_dip_dump_buffer(data32, data);
		if (err) {
			LOG_INF("COMPAT_DIP_DUMP_BUFFER error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, DIP_DUMP_BUFFER, (unsigned long)data);
		return ret;
	}
	case COMPAT_DIP_SET_MEM_INFO: {
		struct compat_DIP_MEM_INFO_STRUCT __user *data32;
		struct DIP_MEM_INFO_STRUCT __user *data;
		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;
		err = compat_get_dip_mem_info(data32, data);
		if (err) {
			LOG_INF("COMPAT_DIP_SET_MEM_INFO error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, DIP_SET_MEM_INFO, (unsigned long)data);
		return ret;
	}
	case DIP_GET_DUMP_INFO:
	case DIP_WAIT_IRQ:
	case DIP_CLEAR_IRQ: /* structure (no pointer) */
	case DIP_REGISTER_IRQ_USER_KEY:
	case DIP_FLUSH_IRQ_REQUEST:
	case DIP_P2_BUFQUE_CTRL:/* structure (no pointer) */
	case DIP_ION_IMPORT:
	case DIP_ION_FREE:
	case DIP_ION_FREE_BY_HWMODULE:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return DIP_ioctl(filep, cmd, arg); */
	}
}

#endif

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_open(
	struct inode *pInode,
	struct file *pFile)
{
	signed int Ret = 0;
	unsigned int i;
	int q = 0;
	struct DIP_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("- E. UserCount: %d.\n", IspInfo.UserCount);


	/*  */
	spin_lock(&(IspInfo.SpinLockIspRef));

	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(struct DIP_USER_INFO_STRUCT), GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)\n",
			current->comm, current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (struct DIP_USER_INFO_STRUCT *)pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
	if (IspInfo.UserCount > 0) {
		IspInfo.UserCount++;
		spin_unlock(&(IspInfo.SpinLockIspRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist\n",
			IspInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else {
		IspInfo.UserCount++;
		spin_unlock(&(IspInfo.SpinLockIspRef));

		/* kernel log limit to (current+150) lines per second */
	#if (_K_LOG_ADJUST == 1)
		DIP_pr_detect_count = get_detect_count();
		i = DIP_pr_detect_count + 150;
		set_detect_count(i);

		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), log_limit_line(%d), first user\n",
			IspInfo.UserCount, current->comm, current->pid, current->tgid, i);
	#else
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), first user\n",
			IspInfo.UserCount, current->comm, current->pid, current->tgid);
	#endif
	}

	/* do wait queue head init when re-enter in camera */
	/*  */
	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		FirstUnusedIrqUserKey = 1;
		strncpy((void *)IrqUserKey_UserInfo[i].userName, "DefaultUserNametoAllocMem", USERKEY_STR_LEN);
		IrqUserKey_UserInfo[i].userKey = -1;
	}
	/*  */
	spin_lock(&(SpinLock_P2FrameList));
	for (q = 0; q < DIP_P2_BUFQUE_PROPERTY_NUM; q++) {
		for (i = 0; i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
			P2_FrameUnit_List[q][i].processID = 0x0;
			P2_FrameUnit_List[q][i].callerID = 0x0;
			P2_FrameUnit_List[q][i].cqMask =  0x0;
			P2_FrameUnit_List[q][i].bufSts = DIP_P2_BUF_STATE_NONE;
		}
		P2_FrameUnit_List_Idx[q].start = 0;
		P2_FrameUnit_List_Idx[q].curr = 0;
		P2_FrameUnit_List_Idx[q].end =  -1;
		/*  */
		for (i = 0; i < _MAX_SUPPORT_P2_PACKAGE_NUM_; i++) {
			P2_FramePackage_List[q][i].processID = 0x0;
			P2_FramePackage_List[q][i].callerID = 0x0;
			P2_FramePackage_List[q][i].dupCQIdx =  -1;
			P2_FramePackage_List[q][i].frameNum = 0;
			P2_FramePackage_List[q][i].dequedNum = 0;
		}
		P2_FramePack_List_Idx[q].start = 0;
		P2_FramePack_List_Idx[q].curr = 0;
		P2_FramePack_List_Idx[q].end =  -1;
	}
	spin_unlock(&(SpinLock_P2FrameList));

	/*  */
	spin_lock((spinlock_t *)(&SpinLockRegScen));
	g_regScen = 0xa5a5a5a5;
	spin_unlock((spinlock_t *)(&SpinLockRegScen));
	/*  */
	IspInfo.BufInfo.Read.pData = kmalloc(DIP_BUF_SIZE, GFP_ATOMIC);
	IspInfo.BufInfo.Read.Size = DIP_BUF_SIZE;
	IspInfo.BufInfo.Read.Status = DIP_BUF_STATUS_EMPTY;
	if (IspInfo.BufInfo.Read.pData == NULL) {
		LOG_DBG("ERROR: BufRead kmalloc failed\n");
		Ret = -ENOMEM;
		goto EXIT;
	}
	g_bIonBufferAllocated = MFALSE;
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
	g_dip_p2_imem_buf.handle = NULL;
	g_dip_p2_imem_buf.ion_fd = 0;
	g_dip_p2_imem_buf.va = 0;
	g_dip_p2_imem_buf.pa = 0;
	g_dip_p2_imem_buf.length = ((4*DIP_DIP_REG_SIZE) + (2*MAX_DIP_TILE_TDR_HEX_NO)
	 + (2*MAX_DIP_CMDQ_BUFFER_SIZE) + (8*0x400));
	dip_p2_ion_client = NULL;
	if ((dip_p2_ion_client == NULL) && (g_ion_device))
		dip_p2_ion_client = ion_client_create(g_ion_device, "dip_p2");
	if (dip_p2_ion_client == NULL)
		LOG_ERR("invalid dip_p2_ion_client client!\n");
	if (dip_allocbuf(&g_dip_p2_imem_buf) >= 0)
		g_bIonBufferAllocated = MTRUE;
#endif

	if (g_bIonBufferAllocated == MTRUE) {
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
		g_pPhyDIPBuffer = (unsigned int *)(uintptr_t)(g_dip_p2_imem_buf.va);
		g_pTuningBuffer = (unsigned int *)(((uintptr_t)g_pPhyDIPBuffer) + DIP_DIP_REG_SIZE);
		g_pTpipeBuffer = (unsigned int *)(((uintptr_t)g_pTuningBuffer) + DIP_DIP_REG_SIZE);
		g_pVirDIPBuffer = (unsigned int *)(((uintptr_t)g_pTpipeBuffer) + MAX_DIP_TILE_TDR_HEX_NO);
		g_pCmdqBuffer = (unsigned int *)(((uintptr_t)g_pVirDIPBuffer) + DIP_DIP_REG_SIZE);
		/* Kernel Exception */
		g_pKWTpipeBuffer = (unsigned int *)(((uintptr_t)g_pCmdqBuffer) + MAX_DIP_CMDQ_BUFFER_SIZE);
		g_pKWCmdqBuffer = (unsigned int *)(((uintptr_t)g_pKWTpipeBuffer) + MAX_DIP_TILE_TDR_HEX_NO);
		g_pKWVirDIPBuffer = (unsigned int *)(((uintptr_t)g_pKWCmdqBuffer) + MAX_DIP_CMDQ_BUFFER_SIZE);
#endif
	} else {
		/* Navtive Exception */
		g_pPhyDIPBuffer = NULL;
		g_pTuningBuffer = NULL;
		g_pTpipeBuffer = NULL;
		g_pVirDIPBuffer = NULL;
		g_pCmdqBuffer = NULL;
		/* Kernel Exception */
		g_pKWTpipeBuffer = NULL;
		g_pKWCmdqBuffer = NULL;
		g_pKWVirDIPBuffer = NULL;
	}
	g_bUserBufIsReady = MFALSE;
	g_bDumpPhyDIPBuf = MFALSE;
	g_dumpInfo.tdri_baseaddr = 0xFFFFFFFF;/* 0x15022204 */
	g_dumpInfo.imgi_baseaddr = 0xFFFFFFFF;/* 0x15022400 */
	g_dumpInfo.dmgi_baseaddr = 0xFFFFFFFF;/* 0x15022520 */
	g_tdriaddr = 0xffffffff;
	g_cmdqaddr = 0xffffffff;
	DumpBufferField = 0;
	g_TpipeBaseAddrInfo.MemInfoCmd = 0x0;
	g_TpipeBaseAddrInfo.MemPa = 0x0;
	g_TpipeBaseAddrInfo.MemVa = NULL;
	g_TpipeBaseAddrInfo.MemSizeDiff = 0x0;
	g_CmdqBaseAddrInfo.MemInfoCmd = 0x0;
	g_CmdqBaseAddrInfo.MemPa = 0x0;
	g_CmdqBaseAddrInfo.MemVa = NULL;
	g_CmdqBaseAddrInfo.MemSizeDiff = 0x0;

	/*  */
	for (i = 0; i < DIP_IRQ_TYPE_AMOUNT; i++) {
		for (q = 0; q < IRQ_USER_NUM_MAX; q++)
			IspInfo.IrqInfo.Status[i][q] = 0;
	}
	/* reset backup regs*/
	memset(g_BkReg, 0, sizeof(struct _dip_bk_reg_t) * DIP_IRQ_TYPE_AMOUNT);

#ifdef ENABLE_KEEP_ION_HANDLE
	/* create ion client*/
	DIP_ion_init();
#endif

#ifdef KERNEL_LOG
	IspInfo.DebugMask = (DIP_DBG_INT);
#endif
	/*  */
EXIT:
	if (Ret < 0) {
		if (IspInfo.BufInfo.Read.pData != NULL) {
			kfree(IspInfo.BufInfo.Read.pData);
			IspInfo.BufInfo.Read.pData = NULL;
		}
	} else {
		/* Enable clock */
		DIP_EnableClock(MTRUE);
#ifdef MMDVFS_PM_QOS_READY
		mmdvfs_pm_qos_add_request(&dip_qos, MMDVFS_PM_QOS_SUB_SYS_CAMERA, 0);
#endif
		LOG_DBG("dip open G_u4EnableClockCount: %d\n", G_u4EnableClockCount);
	}


	LOG_INF("- X. Ret: %d. UserCount: %d.\n", Ret, IspInfo.UserCount);
	return Ret;

}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_release(
	struct inode *pInode,
	struct file *pFile)
{
	struct DIP_USER_INFO_STRUCT *pUserInfo;
	unsigned int i = 0;

	LOG_DBG("- E. UserCount: %d.\n", IspInfo.UserCount);

	/*  */

	/*  */
	/* LOG_DBG("UserCount(%d)",IspInfo.UserCount); */
	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo = (struct DIP_USER_INFO_STRUCT *)pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	/*      */
	spin_lock(&(IspInfo.SpinLockIspRef));
	IspInfo.UserCount--;
	if (IspInfo.UserCount > 0) {
		spin_unlock(&(IspInfo.SpinLockIspRef));
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d),	users exist",
			IspInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	} else {
		spin_unlock(&(IspInfo.SpinLockIspRef));
	}

	/* kernel log limit back to default */
#if (_K_LOG_ADJUST == 1)
	set_detect_count(DIP_pr_detect_count);
#endif
	/*      */
	LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), log_limit_line(%d),	last user",
		IspInfo.UserCount, current->comm, current->pid, current->tgid, DIP_pr_detect_count);

	if (g_bWaitLock == 1) {
#ifdef CONFIG_PM_WAKELOCKS
		__pm_relax(&dip_wake_lock);
#else
		wake_unlock(&dip_wake_lock);
#endif
		g_bWaitLock = 0;
	}
	/* reset */
	/*      */
	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		FirstUnusedIrqUserKey = 1;
		strncpy((void *)IrqUserKey_UserInfo[i].userName, "DefaultUserNametoAllocMem", USERKEY_STR_LEN);
		IrqUserKey_UserInfo[i].userKey = -1;
	}
	if (IspInfo.BufInfo.Read.pData != NULL) {
		kfree(IspInfo.BufInfo.Read.pData);
		IspInfo.BufInfo.Read.pData = NULL;
		IspInfo.BufInfo.Read.Size = 0;
		IspInfo.BufInfo.Read.Status = DIP_BUF_STATUS_EMPTY;
	}
	if (g_bIonBufferAllocated == MFALSE) {
		/* Native Exception */
		if (g_pPhyDIPBuffer != NULL) {
			vfree(g_pPhyDIPBuffer);
			g_pPhyDIPBuffer = NULL;
		}
		if (g_pTuningBuffer != NULL) {
			vfree(g_pTuningBuffer);
			g_pTuningBuffer = NULL;
		}
		if (g_pTpipeBuffer != NULL) {
			vfree(g_pTpipeBuffer);
			g_pTpipeBuffer = NULL;
		}
		if (g_pVirDIPBuffer != NULL) {
			vfree(g_pVirDIPBuffer);
			g_pVirDIPBuffer = NULL;
		}
		if (g_pCmdqBuffer != NULL) {
			vfree(g_pCmdqBuffer);
			g_pCmdqBuffer = NULL;
		}
		/* Kernel Exception */
		if (g_pKWTpipeBuffer != NULL) {
			vfree(g_pKWTpipeBuffer);
			g_pKWTpipeBuffer = NULL;
		}
		if (g_pKWCmdqBuffer != NULL) {
			vfree(g_pKWCmdqBuffer);
			g_pKWCmdqBuffer = NULL;
		}
		if (g_pKWVirDIPBuffer != NULL) {
			vfree(g_pKWVirDIPBuffer);
			g_pKWVirDIPBuffer = NULL;
		}
	} else {
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
		dip_freebuf(&g_dip_p2_imem_buf);
		g_dip_p2_imem_buf.handle = NULL;
		g_dip_p2_imem_buf.ion_fd = 0;
		g_dip_p2_imem_buf.va = 0;
		g_dip_p2_imem_buf.pa = 0;
		g_bIonBufferAllocated = MFALSE;
		/* Navtive Exception */
		g_pPhyDIPBuffer = NULL;
		g_pTuningBuffer = NULL;
		g_pTpipeBuffer = NULL;
		g_pVirDIPBuffer = NULL;
		g_pCmdqBuffer = NULL;
		/* Kernel Exception */
		g_pKWTpipeBuffer = NULL;
		g_pKWCmdqBuffer = NULL;
		g_pKWVirDIPBuffer = NULL;
#endif
	}

#ifdef AEE_DUMP_BY_USING_ION_MEMORY
	if (dip_p2_ion_client != NULL) {
		ion_client_destroy(dip_p2_ion_client);
		dip_p2_ion_client = NULL;
	} else {
		LOG_ERR("dip_p2_ion_client is NULL!!\n");
	}
#endif
	/* reset backup regs*/
	memset(g_BkReg, 0, sizeof(struct _dip_bk_reg_t) * DIP_IRQ_TYPE_AMOUNT);

	/*  */
#ifdef ENABLE_KEEP_ION_HANDLE
	/* free keep ion handles, then destroy ion client*/
	for (i = 0; i < DIP_DEV_NODE_NUM; i++) {
		if (gION_TBL[i].node != DIP_DEV_NODE_NUM)
			DIP_ion_free_handle_by_module(i);
	}

	DIP_ion_uninit();
#endif

	/*  */
	/* LOG_DBG("Before spm_enable_sodi()."); */
	/* Enable sodi (Multi-Core Deep Idle). */

#if 0 /* _mt6593fpga_dvt_use_ */
	spm_enable_sodi();
#endif

EXIT:

	/* Disable clock.
	*  1. clkmgr: G_u4EnableClockCount=0, call clk_enable/disable
	*  2. CCF: call clk_enable/disable every time
	*/
	DIP_EnableClock(MFALSE);

#ifdef MMDVFS_PM_QOS_READY
	mmdvfs_pm_qos_remove_request(&dip_qos);
#endif
	LOG_DBG("dip release G_u4EnableClockCount: %d", G_u4EnableClockCount);

	LOG_INF("- X. UserCount: %d.", IspInfo.UserCount);
	return 0;
}


/*******************************************************************************
*
********************************************************************************/
static signed int DIP_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	unsigned long length = 0;
	unsigned int pfn = 0x0;

	/*LOG_DBG("- E.");*/
	length = (pVma->vm_end - pVma->vm_start);
	/*  */
	pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
	pfn = pVma->vm_pgoff << PAGE_SHIFT;

	/*LOG_INF("DIP_mmap: vm_pgoff(0x%lx),pfn(0x%x),phy(0x%lx),vm_start(0x%lx),vm_end(0x%lx),length(0x%lx)\n",
	*	pVma->vm_pgoff, pfn, pVma->vm_pgoff << PAGE_SHIFT, pVma->vm_start, pVma->vm_end, length);
	*/

	switch (pfn) {
	case DIP_A_BASE_HW:
		if (length > DIP_REG_PER_DIP_RANGE) {
			LOG_ERR("mmap range error :module(0x%x),length(0x%lx),DIP_REG_PER_DIP_RANGE(0x%lx)!\n",
				pfn, length, DIP_REG_PER_DIP_RANGE);
			return -EAGAIN;
		}
		break;
	default:
		LOG_ERR("Illegal starting HW addr for mmap!\n");
		return -EAGAIN;
	}
	if (remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff, pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
		return -EAGAIN;

	/*  */
	return 0;
}

/*******************************************************************************
*
********************************************************************************/

static dev_t IspDevNo;
static struct cdev *pIspCharDrv;
static struct class *pIspClass;

static const struct file_operations IspFileOper = {
	.owner = THIS_MODULE,
	.open = DIP_open,
	.release = DIP_release,
	/* .flush       = mt_dip_flush, */
	.mmap = DIP_mmap,
	.unlocked_ioctl = DIP_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = DIP_ioctl_compat,
#endif
};

/*******************************************************************************
*
********************************************************************************/
static inline void DIP_UnregCharDev(void)
{
	LOG_DBG("- E.");
	/*      */
	/* Release char driver */
	if (pIspCharDrv != NULL) {
		cdev_del(pIspCharDrv);
		pIspCharDrv = NULL;
	}
	/*      */
	unregister_chrdev_region(IspDevNo, 1);
}

/*******************************************************************************
*
********************************************************************************/
static inline signed int DIP_RegCharDev(void)
{
	signed int Ret = 0;
	/*  */
	LOG_DBG("- E.\n");
	/*  */
	Ret = alloc_chrdev_region(&IspDevNo, 0, 1, DIP_DEV_NAME);
	if ((Ret) < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d\n", Ret);
		return Ret;
	}
	/* Allocate driver */
	pIspCharDrv = cdev_alloc();
	if (pIspCharDrv == NULL) {
		LOG_ERR("cdev_alloc failed\n");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/* Attatch file operation. */
	cdev_init(pIspCharDrv, &IspFileOper);
	/*  */
	pIspCharDrv->owner = THIS_MODULE;
	/* Add to system */
	Ret = cdev_add(pIspCharDrv, IspDevNo, 1);
	if ((Ret) < 0) {
		LOG_ERR("Attatch file operation failed, %d\n", Ret);
		goto EXIT;
	}
	/*  */
EXIT:
	if (Ret < 0)
		DIP_UnregCharDev();


	/*      */

	LOG_DBG("- X.\n");
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_probe(struct platform_device *pDev)
{
	signed int Ret = 0;
	/*    struct resource *pRes = NULL;*/
	signed int i = 0, j = 0;
	unsigned char n;
	unsigned int irq_info[3]; /* Record interrupts info from device tree */
	struct dip_device *_dipdev = NULL;

#ifdef CONFIG_OF
	struct dip_device *dip_dev;
	struct device *dev = NULL;
#endif

	LOG_INF("- E. DIP driver probe.\n");

	/* Get platform_device parameters */
#ifdef CONFIG_OF

	if (pDev == NULL) {
		dev_err(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}

	nr_dip_devs += 1;
#if 1
	_dipdev = krealloc(dip_devs, sizeof(struct dip_device) * nr_dip_devs, GFP_KERNEL);
	if (!_dipdev) {
		dev_err(&pDev->dev, "Unable to allocate dip_devs\n");
		return -ENOMEM;
	}
	dip_devs = _dipdev;


	#if defined(DIP_MET_READY)
	/*MET: met mmsys profile*/
	if (met_mmsys_config_isp_base_addr)
		met_mmsys_config_isp_base_addr((unsigned long *)dip_devs);
	#endif


#else
	/* WARNING:KREALLOC_ARG_REUSE:Reusing the krealloc arg is almost always a bug */
	dip_devs = KREALLOC(dip_devs, sizeof(struct dip_device) * nr_dip_devs, GFP_KERNEL);
	if (!dip_devs) {
		dev_err(&pDev->dev, "Unable to allocate dip_devs\n");
		return -ENOMEM;
	}
#endif

	dip_dev = &(dip_devs[nr_dip_devs - 1]);
	dip_dev->dev = &pDev->dev;

	/* iomap registers */
	dip_dev->regs = of_iomap(pDev->dev.of_node, 0);
	if (!dip_dev->regs) {
		dev_err(&pDev->dev, "Unable to ioremap registers, of_iomap fail, nr_dip_devs=%d, devnode(%s).\n",
			nr_dip_devs, pDev->dev.of_node->name);
		return -ENOMEM;
	}

	LOG_INF("nr_dip_devs=%d, devnode(%s), map_addr=0x%lx\n",
		nr_dip_devs, pDev->dev.of_node->name, (unsigned long)dip_dev->regs);

	/* get IRQ ID and request IRQ */
	dip_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);

	if (dip_dev->irq > 0) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array(pDev->dev.of_node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
			dev_err(&pDev->dev, "get irq flags from DTS fail!!\n");
			return -ENODEV;
		}

		for (i = 0; i < DIP_IRQ_TYPE_AMOUNT; i++) {
			if ((strcmp(pDev->dev.of_node->name, DIP_IRQ_CB_TBL[i].device_name) == 0) &&
				(DIP_IRQ_CB_TBL[i].isr_fp != NULL)) {
				Ret = request_irq(dip_dev->irq, (irq_handler_t)DIP_IRQ_CB_TBL[i].isr_fp,
						  irq_info[2], (const char *)DIP_IRQ_CB_TBL[i].device_name, NULL);
				if (Ret) {
					dev_err(&pDev->dev,
					"request_irq fail, nr_dip_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
					nr_dip_devs, pDev->dev.of_node->name,
					dip_dev->irq, DIP_IRQ_CB_TBL[i].device_name);
					return Ret;
				}

				LOG_INF("nr_dip_devs=%d, devnode(%s), irq=%d, ISR: %s\n",
				nr_dip_devs, pDev->dev.of_node->name, dip_dev->irq, DIP_IRQ_CB_TBL[i].device_name);
				break;
			}
		}

		if (i >= DIP_IRQ_TYPE_AMOUNT)
			LOG_INF("No corresponding ISR!!: nr_dip_devs=%d, devnode(%s), irq=%d\n",
				nr_dip_devs, pDev->dev.of_node->name, dip_dev->irq);


	} else {
		LOG_INF("No IRQ!!: nr_dip_devs=%d, devnode(%s), irq=%d\n",
			nr_dip_devs, pDev->dev.of_node->name, dip_dev->irq);
	}



	/* Only register char driver in the 1st time */
	if (nr_dip_devs == 1) {
		/* Register char driver */
		Ret = DIP_RegCharDev();
		if ((Ret)) {
			dev_err(&pDev->dev, "register char failed");
			return Ret;
		}



		/* Create class register */
		pIspClass = class_create(THIS_MODULE, "dipdrv");
		if (IS_ERR(pIspClass)) {
			Ret = PTR_ERR(pIspClass);
			LOG_ERR("Unable to create class, err = %d\n", Ret);
			goto EXIT;
		}
		dev = device_create(pIspClass, NULL, IspDevNo, NULL, DIP_DEV_NAME);

		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_err(&pDev->dev, "Failed to create device: /dev/%s, err = %d", DIP_DEV_NAME, Ret);
			goto EXIT;
		}

#endif

		/* Init spinlocks */
		spin_lock_init(&(IspInfo.SpinLockIspRef));
		spin_lock_init(&(IspInfo.SpinLockIsp));
		for (n = 0; n < DIP_IRQ_TYPE_AMOUNT; n++) {
			spin_lock_init(&(IspInfo.SpinLockIrq[n]));
			spin_lock_init(&(IspInfo.SpinLockIrqCnt[n]));
		}
		spin_lock_init(&(IspInfo.SpinLockRTBC));
		spin_lock_init(&(IspInfo.SpinLockClock));

		spin_lock_init(&(SpinLock_P2FrameList));
		spin_lock_init(&(SpinLockRegScen));
		spin_lock_init(&(SpinLock_UserKey));
		#ifdef ENABLE_KEEP_ION_HANDLE
		for (i = 0; i < DIP_DEV_NODE_NUM; i++) {
			if (gION_TBL[i].node != DIP_DEV_NODE_NUM) {
				for (n = 0; n < _dma_max_wr_; n++)
					spin_lock_init(&(gION_TBL[i].pLock[n]));
			}
		}
		#endif

#ifdef EP_NO_CLKMGR


#else
		/*CCF: Grab clock pointer (struct clk*) */
		dip_clk.DIP_IMG_LARB5 = devm_clk_get(&pDev->dev, "DIP_CG_IMG_LARB5");
		dip_clk.DIP_IMG_LARB2 = devm_clk_get(&pDev->dev, "DIP_CG_IMG_LARB2");
		dip_clk.DIP_IMG_DIP = devm_clk_get(&pDev->dev, "DIP_CG_IMG_DIP");

		if (IS_ERR(dip_clk.DIP_IMG_LARB5)) {
			LOG_ERR("cannot get DIP_IMG_LARB5 clock\n");
			return PTR_ERR(dip_clk.DIP_IMG_LARB5);
		}
		if (IS_ERR(dip_clk.DIP_IMG_LARB2)) {
			LOG_ERR("cannot get DIP_IMG_LARB2 clock\n");
			return PTR_ERR(dip_clk.DIP_IMG_LARB2);
		}
		if (IS_ERR(dip_clk.DIP_IMG_DIP)) {
			LOG_ERR("cannot get DIP_IMG_DIP clock\n");
			return PTR_ERR(dip_clk.DIP_IMG_DIP);
		}

#endif
		/*  */
		for (i = 0 ; i < DIP_IRQ_TYPE_AMOUNT; i++)
			init_waitqueue_head(&IspInfo.WaitQueueHead[i]);

#ifdef CONFIG_PM_WAKELOCKS
		wakeup_source_init(&dip_wake_lock, "dip_lock_wakelock");
#else
		wake_lock_init(&dip_wake_lock, WAKE_LOCK_SUSPEND, "dip_lock_wakelock");
#endif

		/* enqueue/dequeue control in ihalpipe wrapper */
		init_waitqueue_head(&P2WaitQueueHead_WaitDeque);
		init_waitqueue_head(&P2WaitQueueHead_WaitFrame);
		init_waitqueue_head(&P2WaitQueueHead_WaitFrameEQDforDQ);

		for (i = 0; i < DIP_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(dip_tasklet[i].pIsp_tkt, dip_tasklet[i].tkt_cb, 0);

#if (DIP_BOTTOMHALF_WORKQ == 1)
		for (i = 0 ; i < DIP_IRQ_TYPE_AMOUNT; i++) {
			dip_workque[i].module = i;
			memset((void *)&(dip_workque[i].dip_bh_work), 0,
				sizeof(dip_workque[i].dip_bh_work));
			INIT_WORK(&(dip_workque[i].dip_bh_work), DIP_BH_Workqueue);
		}
#endif


		/* Init IspInfo*/
		spin_lock(&(IspInfo.SpinLockIspRef));
		IspInfo.UserCount = 0;
		spin_unlock(&(IspInfo.SpinLockIspRef));
		/*  */
		/* Init IrqCntInfo */
		for (i = 0; i < DIP_IRQ_TYPE_AMOUNT; i++) {
			for (j = 0; j < DIP_ISR_MAX_NUM; j++) {
				IspInfo.IrqCntInfo.m_err_int_cnt[i][j] = 0;
				IspInfo.IrqCntInfo.m_warn_int_cnt[i][j] = 0;
			}
			IspInfo.IrqCntInfo.m_err_int_mark[i] = 0;
			IspInfo.IrqCntInfo.m_warn_int_mark[i] = 0;

			IspInfo.IrqCntInfo.m_int_usec[i] = 0;
		}


EXIT:
		if (Ret < 0)
			DIP_UnregCharDev();

	}

	LOG_INF("- X. DIP driver probe.\n");

	return Ret;
}

/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static signed int DIP_remove(struct platform_device *pDev)
{
	/*    struct resource *pRes;*/
	signed int IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	/* unregister char driver. */
	DIP_UnregCharDev();

	/* Release IRQ */
	disable_irq(IspInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* kill tasklet */
	for (i = 0; i < DIP_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(dip_tasklet[i].pIsp_tkt);

#if 0
	/* free all registered irq(child nodes) */
	DIP_UnRegister_AllregIrq();
	/* free father nodes of irq user list */
	struct my_list_head *head;
	struct my_list_head *father;

	head = ((struct my_list_head *)(&SupIrqUserListHead.list));
	while (1) {
		father = head;
		if (father->nextirq != father) {
			father = father->nextirq;
			REG_IRQ_NODE *accessNode;

			typeof(((REG_IRQ_NODE *)0)->list) * __mptr = (father);
			accessNode = ((REG_IRQ_NODE *)((char *)__mptr - offsetof(REG_IRQ_NODE, list)));
			LOG_INF("free father,reg_T(%d)\n", accessNode->reg_T);
			if (father->nextirq != father) {
				head->nextirq = father->nextirq;
				father->nextirq = father;
			} else {
				/* last father node */
				head->nextirq = head;
				LOG_INF("break\n");
				break;
			}
			kfree(accessNode);
		}
	}
#endif
	/*  */
	device_destroy(pIspClass, IspDevNo);
	/*  */
	class_destroy(pIspClass);
	pIspClass = NULL;
	/*  */
	return 0;
}

static signed int DIP_suspend(
	struct platform_device *pDev,
	pm_message_t            Mesg
)
{
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static signed int DIP_resume(struct platform_device *pDev)
{
	return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int DIP_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	/*pr_debug("calling %s()\n", __func__);*/

	return DIP_suspend(pdev, PMSG_SUSPEND);
}

int DIP_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	/*pr_debug("calling %s()\n", __func__);*/

	return DIP_resume(pdev);
}

/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
int DIP_pm_restore_noirq(struct device *device)
{
	/*pr_debug("calling %s()\n", __func__);*/
#ifndef CONFIG_OF
	mt_irq_set_sens(CAM0_IRQ_BIT_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(CAM0_IRQ_BIT_ID, MT_POLARITY_LOW);
#endif
	return 0;

}
/*---------------------------------------------------------------------------*/
#else /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
#define DIP_pm_suspend NULL
#define DIP_pm_resume  NULL
#define DIP_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/

const struct dev_pm_ops DIP_pm_ops = {
	.suspend = DIP_pm_suspend,
	.resume = DIP_pm_resume,
	.freeze = DIP_pm_suspend,
	.thaw = DIP_pm_resume,
	.poweroff = DIP_pm_suspend,
	.restore = DIP_pm_resume,
	.restore_noirq = DIP_pm_restore_noirq,
};


/*******************************************************************************
*
********************************************************************************/
static struct platform_driver DipDriver = {
	.probe   = DIP_probe,
	.remove  = DIP_remove,
	.suspend = DIP_suspend,
	.resume  = DIP_resume,
	.driver  = {
		.name  = DIP_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = dip_of_ids,
#endif
#ifdef CONFIG_PM
		.pm     = &DIP_pm_ops,
#endif
	}
};

/*******************************************************************************
*
********************************************************************************/
/*
* ssize_t (*read) (struct file *, char __user *, size_t, loff_t *)
*/
#define USE_OLD_STYPE_11897 0
#if USE_OLD_STYPE_11897
static signed int DIP_DumpRegToProc(
	char *pPage,
	char **ppStart,
	off_t off,
	signed int Count,
	signed int *pEof,
	void *pData)
#else /* new file_operations style */
static ssize_t DIP_DumpRegToProc(
	struct file *pFile,
	char *pStart,
	size_t off,
	loff_t *Count)
#endif
{
#if USE_OLD_STYPE_11897
	char *p = pPage;
	signed int Length = 0;
	unsigned int i = 0;
	signed int ret = 0;
	/*  */
	LOG_DBG("- E. pPage: %p. off: %d. Count: %d.", pPage, (unsigned int)off, Count);
	/*  */
	p += sprintf(p, " MT6593 DIP Register\n");
	p += sprintf(p, "====== top ====\n");
	for (i = 0x0; i <= 0x1AC; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	p += sprintf(p, "====== dma ====\n");
	for (i = 0x200; i <= 0x3D8; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n\r", (unsigned int)(DIP_ADDR + i),
			(unsigned int)DIP_RD32(DIP_ADDR + i));

	p += sprintf(p, "====== tg ====\n");
	for (i = 0x400; i <= 0x4EC; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	p += sprintf(p, "====== cdp (including EIS) ====\n");
	for (i = 0xB00; i <= 0xDE0; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	p += sprintf(p, "====== seninf ====\n");
	for (i = 0x4000; i <= 0x40C0; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4100; i <= 0x41BC; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4200; i <= 0x4208; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4300; i <= 0x4310; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x43A0; i <= 0x43B0; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4400; i <= 0x4424; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4500; i <= 0x4520; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4600; i <= 0x4608; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	for (i = 0x4A00; i <= 0x4A08; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	p += sprintf(p, "====== 3DNR ====\n");
	for (i = 0x4F00; i <= 0x4F38; i += 4)
		p += sprintf(p, "+0x%08x 0x%08x\n", (unsigned int)(DIP_ADDR + i), (unsigned int)DIP_RD32(DIP_ADDR + i));

	/*  */
	*ppStart = pPage + off;
	/*  */
	Length = p - pPage;
	if (Length > off)
		Length -= off;
	else
		Length = 0;

	/*  */

	ret = Length < Count ? Length : Count;

	LOG_DBG("- X. ret: %d.", ret);

	return ret;
#else /* new file_operations style */
	LOG_ERR("DIP_DumpRegToProc: Not implement");
	return 0;
#endif
}

/*******************************************************************************
*
********************************************************************************/
/*
* ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *)
*/
#define USE_OLD_STYPE_12011 0
#if USE_OLD_STYPE_12011
static signed int  DIP_RegDebug(
	struct file *pFile,
	const char *pBuffer,
	unsigned long   Count,
	void *pData)
#else /* new file_operations style */
static ssize_t DIP_RegDebug(
	struct file *pFile,
	const char *pBuffer,
	size_t Count,
	loff_t *pData)
#endif
{
	LOG_ERR("DIP_RegDebug: Not implement");
	return 0;
}

/*
* ssize_t (*read) (struct file *, char __user *, size_t, loff_t *)
*/
#define USE_OLD_STYPE_12061 0
#if USE_OLD_STYPE_12061
static unsigned int proc_regOfst;
static signed int CAMIO_DumpRegToProc(
	char *pPage,
	char **ppStart,
	off_t   off,
	signed int  Count,
	signed int *pEof,
	void *pData)
#else /* new file_operations style */
static ssize_t CAMIO_DumpRegToProc(
	struct file *pFile,
	char *pStart,
	size_t off,
	loff_t *Count)
#endif
{
	LOG_ERR("CAMIO_DumpRegToProc: Not implement");
	return 0;
}


/*******************************************************************************
*
********************************************************************************/
/*
* ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *)
*/
#define USE_OLD_STYPE_12112 0
#if USE_OLD_STYPE_12112
static signed int  CAMIO_RegDebug(
	struct file *pFile,
	const char *pBuffer,
	unsigned long   Count,
	void *pData)
#else /* new file_operations style */
static ssize_t CAMIO_RegDebug(
	struct file *pFile,
	const char *pBuffer,
	size_t Count,
	loff_t *pData)
#endif
{
	LOG_ERR("CAMIO_RegDebug: Not implement");
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static int dip_p2_ke_dump_read(struct seq_file *m, void *v)
{
#ifdef AEE_DUMP_REDUCE_MEMORY
	int i;

	LOG_INF("dip p2 ke dump start!! g_bDumpPhyDIPBuf:%d\n", g_bDumpPhyDIPBuf);
	LOG_INF("g_bDumpPhyDIPBuf:%d, g_tdriaddr:0x%x, g_cmdqaddr:0x%x\n",
		g_bDumpPhyDIPBuf, g_tdriaddr, g_cmdqaddr);
	seq_puts(m, "============ dip p2 ke dump register============\n");
	seq_printf(m, "===dip p2 you can trust below info: g_bDumpPhyDIPBuf:%d===\n", g_bDumpPhyDIPBuf);
	seq_printf(m, "===dip p2 g_bDumpPhyDIPBuf:%d, g_tdriaddr:0x%x, g_cmdqaddr:0x%x===\n", g_bDumpPhyDIPBuf,
		g_tdriaddr, g_cmdqaddr);
	seq_puts(m, "===dip p2 hw physical register===\n");
	if (g_bDumpPhyDIPBuf == MFALSE)
		return 0;
	if (g_pPhyDIPBuffer != NULL) {
		for (i = 0; i < (DIP_DIP_PHYSICAL_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
					DIP_A_BASE_HW+4*i, (unsigned int)g_pPhyDIPBuffer[i],
					DIP_A_BASE_HW+4*(i+1), (unsigned int)g_pPhyDIPBuffer[i+1],
					DIP_A_BASE_HW+4*(i+2), (unsigned int)g_pPhyDIPBuffer[i+2],
					DIP_A_BASE_HW+4*(i+3), (unsigned int)g_pPhyDIPBuffer[i+3]);
		}
	} else {
		LOG_INF("g_pPhyDIPBuffer:(0x%pK)\n", g_pPhyDIPBuffer);
	}
	seq_puts(m, "===dip p2 tpipe buffer Info===\n");
	if (g_pKWTpipeBuffer != NULL) {
		for (i = 0; i < (MAX_DIP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
			seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
					(unsigned int)g_pKWTpipeBuffer[i],
					(unsigned int)g_pKWTpipeBuffer[i+1],
					(unsigned int)g_pKWTpipeBuffer[i+2],
					(unsigned int)g_pKWTpipeBuffer[i+3]);
		}
	}
	seq_puts(m, "===dip p2 cmdq buffer Info===\n");
	if (g_pKWCmdqBuffer != NULL) {
		for (i = 0; i < (MAX_DIP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
			seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
					(unsigned int)g_pKWCmdqBuffer[i],
					(unsigned int)g_pKWCmdqBuffer[i+1],
					(unsigned int)g_pKWCmdqBuffer[i+2],
					(unsigned int)g_pKWCmdqBuffer[i+3]);
		}
	}
	seq_puts(m, "===dip p2 vir dip buffer Info===\n");
	if (g_pKWVirDIPBuffer != NULL) {
		for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
					DIP_A_BASE_HW+4*i, (unsigned int)g_pKWVirDIPBuffer[i],
					DIP_A_BASE_HW+4*(i+1), (unsigned int)g_pKWVirDIPBuffer[i+1],
					DIP_A_BASE_HW+4*(i+2), (unsigned int)g_pKWVirDIPBuffer[i+2],
					DIP_A_BASE_HW+4*(i+3), (unsigned int)g_pKWVirDIPBuffer[i+3]);
		}
	}
	seq_puts(m, "============ dip p2 ke dump debug ============\n");
	LOG_INF("dip p2 ke dump end\n");
#else
	int i;

	seq_puts(m, "============ dip p2 ke dump register============\n");
	seq_puts(m, "dip p2 hw physical register\n");
	for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
			   DIP_A_BASE_HW+4*i, (unsigned int)g_PhyDIPBuffer[i],
			   DIP_A_BASE_HW+4*(i+1), (unsigned int)g_PhyDIPBuffer[i+1],
			   DIP_A_BASE_HW+4*(i+2), (unsigned int)g_PhyDIPBuffer[i+2],
			   DIP_A_BASE_HW+4*(i+3), (unsigned int)g_PhyDIPBuffer[i+3]);
	}
	seq_puts(m, "dip p2 tpipe buffer Info\n");
	for (i = 0; i < (MAX_DIP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
		seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
				(unsigned int)g_KWTpipeBuffer[i],
				(unsigned int)g_KWTpipeBuffer[i+1],
				(unsigned int)g_KWTpipeBuffer[i+2],
				(unsigned int)g_KWTpipeBuffer[i+3]);
	}
	seq_puts(m, "dip p2 cmdq buffer Info\n");
	for (i = 0; i < (MAX_DIP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
				(unsigned int)g_KWCmdqBuffer[i],
				(unsigned int)g_KWCmdqBuffer[i+1],
				(unsigned int)g_KWCmdqBuffer[i+2],
				(unsigned int)g_KWCmdqBuffer[i+3]);
	}
	seq_puts(m, "dip p2 vir dip buffer Info\n");
	for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
				DIP_A_BASE_HW+4*i, (unsigned int)g_KWVirDIPBuffer[i],
				DIP_A_BASE_HW+4*(i+1), (unsigned int)g_KWVirDIPBuffer[i+1],
				DIP_A_BASE_HW+4*(i+2), (unsigned int)g_KWVirDIPBuffer[i+2],
				DIP_A_BASE_HW+4*(i+3), (unsigned int)g_KWVirDIPBuffer[i+3]);
	}
	seq_puts(m, "============ dip p2 ke dump debug ============\n");
#endif
	return 0;
}
static int proc_dip_p2_ke_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, dip_p2_ke_dump_read, NULL);
}
static const struct file_operations dip_p2_ke_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_dip_p2_ke_dump_open,
	.read = seq_read,
};

/*******************************************************************************
*
********************************************************************************/
static int dip_p2_dump_read(struct seq_file *m, void *v)
{
#ifdef AEE_DUMP_REDUCE_MEMORY
	int i;

	LOG_INF("dip p2 ne dump start!! g_bUserBufIsReady:%d, g_bIonBufferAllocated:%d\n",
		g_bUserBufIsReady, g_bIonBufferAllocated);
	LOG_INF("dip p2 g_bDumpPhyB:%d, tdriadd:0x%x,imgiadd:0x%x,dmgiadd:0x%x\n",
		g_bDumpPhyDIPBuf, g_dumpInfo.tdri_baseaddr, g_dumpInfo.imgi_baseaddr, g_dumpInfo.dmgi_baseaddr);
	seq_puts(m, "============ dip p2 ne dump register============\n");
	seq_printf(m, "===dip p2 you can trust below info:UserBufIsReady:%d===\n", g_bUserBufIsReady);
	seq_printf(m, "===dip p2 g_bDumpPhyB:%d,tdriadd:0x%x,imgiadd:0x%x,dmgiadd:0x%x===\n",
		g_bDumpPhyDIPBuf, g_dumpInfo.tdri_baseaddr, g_dumpInfo.imgi_baseaddr, g_dumpInfo.dmgi_baseaddr);
	seq_puts(m, "===dip p2 hw physical register===\n");
	if (g_bUserBufIsReady == MFALSE)
		return 0;
	if (g_pPhyDIPBuffer != NULL) {
		for (i = 0; i < (DIP_DIP_PHYSICAL_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
					DIP_A_BASE_HW+4*i, (unsigned int)g_pPhyDIPBuffer[i],
					DIP_A_BASE_HW+4*(i+1), (unsigned int)g_pPhyDIPBuffer[i+1],
					DIP_A_BASE_HW+4*(i+2), (unsigned int)g_pPhyDIPBuffer[i+2],
					DIP_A_BASE_HW+4*(i+3), (unsigned int)g_pPhyDIPBuffer[i+3]);
		}
	} else {
		LOG_INF("g_pPhyDIPBuffer:(0x%pK)\n", g_pPhyDIPBuffer);
	}
	seq_puts(m, "===dip p2 tpipe buffer Info===\n");
	if (g_pTpipeBuffer != NULL) {
		for (i = 0; i < (MAX_DIP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
			seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
					(unsigned int)g_pTpipeBuffer[i],
					(unsigned int)g_pTpipeBuffer[i+1],
					(unsigned int)g_pTpipeBuffer[i+2],
					(unsigned int)g_pTpipeBuffer[i+3]);
		}
	}
	seq_puts(m, "===dip p2 cmdq buffer Info===\n");
	if (g_pCmdqBuffer != NULL) {
		for (i = 0; i < (MAX_DIP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
			seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
					(unsigned int)g_pCmdqBuffer[i],
					(unsigned int)g_pCmdqBuffer[i+1],
					(unsigned int)g_pCmdqBuffer[i+2],
					(unsigned int)g_pCmdqBuffer[i+3]);
		}
	}
	seq_puts(m, "===dip p2 vir dip buffer Info===\n");
	if (g_pVirDIPBuffer != NULL) {
		for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
					DIP_A_BASE_HW+4*i, (unsigned int)g_pVirDIPBuffer[i],
					DIP_A_BASE_HW+4*(i+1), (unsigned int)g_pVirDIPBuffer[i+1],
					DIP_A_BASE_HW+4*(i+2), (unsigned int)g_pVirDIPBuffer[i+2],
					DIP_A_BASE_HW+4*(i+3), (unsigned int)g_pVirDIPBuffer[i+3]);
		}
	}
	seq_puts(m, "===dip p2 tuning buffer Info===\n");
	if (g_pTuningBuffer != NULL) {
		for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
					DIP_A_BASE_HW+4*i, (unsigned int)g_pTuningBuffer[i],
					DIP_A_BASE_HW+4*(i+1), (unsigned int)g_pTuningBuffer[i+1],
					DIP_A_BASE_HW+4*(i+2), (unsigned int)g_pTuningBuffer[i+2],
					DIP_A_BASE_HW+4*(i+3), (unsigned int)g_pTuningBuffer[i+3]);
		}
	}
	seq_puts(m, "============ dip p2 ne dump debug ============\n");
	LOG_INF("dip p2 ne dump end\n");
#else
	int i;

	seq_puts(m, "============ dip p2 ne dump register============\n");
	seq_puts(m, "dip p2 hw physical register\n");
	for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
				DIP_A_BASE_HW+4*i, (unsigned int)g_PhyDIPBuffer[i],
				DIP_A_BASE_HW+4*(i+1), (unsigned int)g_PhyDIPBuffer[i+1],
				DIP_A_BASE_HW+4*(i+2), (unsigned int)g_PhyDIPBuffer[i+2],
				DIP_A_BASE_HW+4*(i+3), (unsigned int)g_PhyDIPBuffer[i+3]);
	}

	seq_puts(m, "dip p2 tpipe buffer Info\n");
	for (i = 0; i < (MAX_DIP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
		seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
			   (unsigned int)g_TpipeBuffer[i],
			   (unsigned int)g_TpipeBuffer[i+1],
			   (unsigned int)g_TpipeBuffer[i+2],
			   (unsigned int)g_TpipeBuffer[i+3]);
	}

	seq_puts(m, "dip p2 cmdq buffer Info\n");
	for (i = 0; i < (MAX_DIP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
			   (unsigned int)g_CmdqBuffer[i],
			   (unsigned int)g_CmdqBuffer[i+1],
			   (unsigned int)g_CmdqBuffer[i+2],
			   (unsigned int)g_CmdqBuffer[i+3]);
	}

	seq_puts(m, "dip p2 vir dip buffer Info\n");
	for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
			   DIP_A_BASE_HW+4*i, (unsigned int)g_VirDIPBuffer[i],
			   DIP_A_BASE_HW+4*(i+1), (unsigned int)g_VirDIPBuffer[i+1],
			   DIP_A_BASE_HW+4*(i+2), (unsigned int)g_VirDIPBuffer[i+2],
			   DIP_A_BASE_HW+4*(i+3), (unsigned int)g_VirDIPBuffer[i+3]);
	}
	seq_puts(m, "dip p2 tuning buffer Info\n");
	for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
				DIP_A_BASE_HW+4*i, (unsigned int)g_TuningBuffer[i],
				DIP_A_BASE_HW+4*(i+1), (unsigned int)g_TuningBuffer[i+1],
				DIP_A_BASE_HW+4*(i+2), (unsigned int)g_TuningBuffer[i+2],
				DIP_A_BASE_HW+4*(i+3), (unsigned int)g_TuningBuffer[i+3]);
	}
	seq_puts(m, "============ kernel warning ============\n");
	seq_puts(m, "ke:dip p2 tpipe buffer Info\n");
	for (i = 0; i < (MAX_DIP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
		seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
				(unsigned int)g_KWTpipeBuffer[i],
				(unsigned int)g_KWTpipeBuffer[i+1],
				(unsigned int)g_KWTpipeBuffer[i+2],
				(unsigned int)g_KWTpipeBuffer[i+3]);
	}
	seq_puts(m, "ke:dip p2 cmdq buffer Info\n");
	for (i = 0; i < (MAX_DIP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
				(unsigned int)g_KWCmdqBuffer[i],
				(unsigned int)g_KWCmdqBuffer[i+1],
				(unsigned int)g_KWCmdqBuffer[i+2],
				(unsigned int)g_KWCmdqBuffer[i+3]);
	}
	seq_puts(m, "ke:dip p2 vir dip buffer Info\n");
	for (i = 0; i < (DIP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n",
				DIP_A_BASE_HW+4*i, (unsigned int)g_KWVirDIPBuffer[i],
				DIP_A_BASE_HW+4*(i+1), (unsigned int)g_KWVirDIPBuffer[i+1],
				DIP_A_BASE_HW+4*(i+2), (unsigned int)g_KWVirDIPBuffer[i+2],
				DIP_A_BASE_HW+4*(i+3), (unsigned int)g_KWVirDIPBuffer[i+3]);
	}
	seq_puts(m, "============ dip p2 ne dump debug ============\n");
#endif
	return 0;
}

static int proc_dip_p2_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, dip_p2_dump_read, NULL);
}

static const struct file_operations dip_p2_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_dip_p2_dump_open,
	.read = seq_read,
};
/*******************************************************************************
*
********************************************************************************/
static const struct file_operations fcameradip_proc_fops = {
	.read = DIP_DumpRegToProc,
	.write = DIP_RegDebug,
};
static const struct file_operations fcameraio_proc_fops = {
	.read = CAMIO_DumpRegToProc,
	.write = CAMIO_RegDebug,
};
/*******************************************************************************
*
********************************************************************************/

static signed int __init DIP_Init(void)
{
	signed int Ret = 0, j;
	void *tmp;
	struct device_node *node = NULL;
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *dip_p2_dir;

	int i;
	/*  */
	LOG_DBG("- E. Magic: %d", DIP_MAGIC);
	/*  */
	Ret = platform_driver_register(&DipDriver);
	if ((Ret) < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}
	/*  */

	/* Use of_find_compatible_node() sensor registers from device tree */
	/* Don't use compatitble define in probe(). Otherwise, probe() of Seninf driver cannot be called. */
	node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
	if (!node) {
		LOG_ERR("find mediatek,apmixed node failed!!!\n");
		return -ENODEV;
	}
	DIP_CLOCK_CELL_BASE = of_iomap(node, 0);
	if (!DIP_CLOCK_CELL_BASE) {
		LOG_ERR("unable to map DIP_CLOCK_CELL_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("DIP_CLOCK_CELL_BASE: %p\n", DIP_CLOCK_CELL_BASE);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mmsys_config");
	if (!node) {
		LOG_ERR("find mmsys_config node failed!!!\n");
		return -ENODEV;
	}
	DIP_MMSYS_CONFIG_BASE = of_iomap(node, 0);
	if (!DIP_MMSYS_CONFIG_BASE) {
		LOG_ERR("unable to map DIP_MMSYS_CONFIG_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("DIP_MMSYS_CONFIG_BASE: %p\n", DIP_MMSYS_CONFIG_BASE);

	/* FIX-ME: linux-3.10 procfs API changed */
	proc_create("driver/dip_reg", 0, NULL, &fcameradip_proc_fops);

	dip_p2_dir = proc_mkdir("dip_p2", NULL);
	if (!dip_p2_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/dip_p2\n", __func__);
		return 0;
	}
	proc_entry = proc_create("dip_p2_dump", S_IRUGO, dip_p2_dir, &dip_p2_dump_proc_fops);
	proc_entry = proc_create("dip_p2_kedump", S_IRUGO, dip_p2_dir, &dip_p2_ke_dump_proc_fops);
	for (j = 0; j < DIP_IRQ_TYPE_AMOUNT; j++) {
		switch (j) {
		default:
			pBuf_kmalloc[j] = NULL;
			pTbl_RTBuf[j] = NULL;
			Tbl_RTBuf_MMPSize[j] = 0;
			break;
		}
	}


	/* isr log */
	if (PAGE_SIZE < ((DIP_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN * ((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1))*LOG_PPNUM)) {
		i = 0;
		while (i < ((DIP_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN * ((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1))*LOG_PPNUM))
			i += PAGE_SIZE;

	} else {
		i = PAGE_SIZE;
	}
	pLog_kmalloc = kmalloc(i, GFP_KERNEL);
	if ((pLog_kmalloc) == NULL) {
		LOG_ERR("mem not enough\n");
		return -ENOMEM;
	}
	memset(pLog_kmalloc, 0x00, i);
	tmp = pLog_kmalloc;
	for (i = 0; i < LOG_PPNUM; i++) {
		for (j = 0; j < DIP_IRQ_TYPE_AMOUNT; j++) {
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
		tmp = (void *)((char *)tmp + NORMAL_STR_LEN);  /* log buffer ,in case of overflow */
	}
	/* mark the pages reserved , FOR MMAP*/
	for (j = 0; j < DIP_IRQ_TYPE_AMOUNT; j++) {
		if (pTbl_RTBuf[j] != NULL) {
			for (i = 0; i < Tbl_RTBuf_MMPSize[j] * PAGE_SIZE; i += PAGE_SIZE)
				SetPageReserved(virt_to_page(((unsigned long)pTbl_RTBuf[j]) + i));

		}
	}

#ifndef EP_CODE_MARK_CMDQ
	/* Register DIP callback */
	LOG_DBG("register dip callback for MDP");
	cmdqCoreRegisterCB(CMDQ_GROUP_ISP,
			   DIP_MDPClockOnCallback,
			   DIP_MDPDumpCallback,
			   DIP_MDPResetCallback,
			   DIP_MDPClockOffCallback);
	/* Register GCE callback for dumping DIP register */
	LOG_DBG("register dip callback for GCE");
	cmdqCoreRegisterDebugRegDumpCB(DIP_BeginGCECallback, DIP_EndGCECallback);
#endif
	/* m4u_enable_tf(M4U_PORT_CAM_IMGI, 0);*/

	for (i = 0; i < DIP_DEV_NODE_NUM; i++)
		SuspnedRecord[i] = 0;

	LOG_DBG("- X. Ret: %d.", Ret);
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static void __exit DIP_Exit(void)
{
	int i, j;

	LOG_DBG("- E.");
	/*  */
	platform_driver_unregister(&DipDriver);
	/*  */
#ifndef EP_CODE_MARK_CMDQ
	/* Unregister DIP callback */
	cmdqCoreRegisterCB(CMDQ_GROUP_ISP,
			   NULL,
			   NULL,
			   NULL,
			   NULL);
	/* Un-Register GCE callback */
	LOG_DBG("Un-register dip callback for GCE");
	cmdqCoreRegisterDebugRegDumpCB(NULL, NULL);
#endif


	for (j = 0; j < DIP_IRQ_TYPE_AMOUNT; j++) {
		if (pTbl_RTBuf[j] != NULL) {
			/* unreserve the pages */
			for (i = 0; i < Tbl_RTBuf_MMPSize[j] * PAGE_SIZE; i += PAGE_SIZE)
				ClearPageReserved(virt_to_page(((unsigned long)pTbl_RTBuf[j]) + i));

			/* free the memory areas */
			kfree(pBuf_kmalloc[j]);
		}
	}

	/* free the memory areas */
	kfree(pLog_kmalloc);

	/*  */
}

int32_t DIP_MDPClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("DIP_MDPClockOnCallback"); */
	/*LOG_DBG("+MDPEn:%d", G_u4EnableClockCount);*/
	DIP_EnableClock(MTRUE);

	return 0;
}

int32_t DIP_MDPDumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("DIP_MDPDumpCallback");

	DIP_DumpDIPReg();

	return 0;
}
int32_t DIP_MDPResetCallback(uint64_t engineFlag)
{
	LOG_DBG("DIP_MDPResetCallback");

	DIP_Reset(DIP_REG_SW_CTL_RST_CAM_P2);

	return 0;
}

int32_t DIP_MDPClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("DIP_MDPClockOffCallback"); */
	DIP_EnableClock(MFALSE);
	/*LOG_DBG("-MDPEn:%d", G_u4EnableClockCount);*/
	return 0;
}


#define DIP_IMGSYS_BASE_PHY_KK 0x15022000

static uint32_t addressToDump[] = {
#if 1
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x000),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x004),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x008),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x00C),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x010),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x014),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x018),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x204),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x208),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x20C),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x400),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x408),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x410),
(uint32_t)(DIP_IMGSYS_BASE_PHY_KK + 0x414),
#endif
};

int32_t DIP_BeginGCECallback(uint32_t taskID, uint32_t *regCount, uint32_t **regAddress)
{
	LOG_DBG("+,taskID(%d)", taskID);

	*regCount = sizeof(addressToDump) / sizeof(uint32_t);
	*regAddress = (uint32_t *)addressToDump;

	LOG_DBG("-,*regCount(%d)", *regCount);

	return 0;
}

int32_t DIP_EndGCECallback(uint32_t taskID, uint32_t regCount, uint32_t *regValues)
{
	#define PER_LINE_LOG_SIZE   10
	int32_t i, j, pos;
	/* uint32_t add[PER_LINE_LOG_SIZE]; */
	uint32_t add[PER_LINE_LOG_SIZE];
	uint32_t val[PER_LINE_LOG_SIZE];

#if DUMP_GCE_TPIPE
	int32_t tpipePA;
	int32_t ctlStart;
	unsigned long map_va = 0;
	uint32_t map_size;
	int32_t *pMapVa;
	#define TPIPE_DUMP_SIZE    200
#endif

	LOG_DBG("End taskID(%d),regCount(%d)", taskID, regCount);

	for (i = 0; i < regCount; i += PER_LINE_LOG_SIZE) {
		for (j = 0; j < PER_LINE_LOG_SIZE; j++) {
			pos = i + j;
			if (pos < regCount) {
				/* addr[j] = (uint32_t)addressToDump[pos]&0xffff; */
				add[j] = addressToDump[pos];
				val[j] = regValues[pos];
			}
		}

		LOG_DBG("[0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x]\n",
			add[0], val[0], add[1], val[1], add[2], val[2], add[3], val[3], add[4], val[4]);
		LOG_DBG("[0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x]\n",
			add[5], val[5], add[6], val[6], add[7], val[7], add[8], val[8], add[9], val[9]);
	}

#if DUMP_GCE_TPIPE
	/* tpipePA = DIP_RD32(DIP_IMGSYS_BASE_PHY_KK + 0x204); */
	tpipePA = val[7];
	/* ctlStart = DIP_RD32(DIP_IMGSYS_BASE_PHY_KK + 0x000); */
	ctlStart = val[0];

	LOG_DBG("kk:tpipePA(0x%x), ctlStart(0x%x)", tpipePA, ctlStart);

	if ((tpipePA)) {
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
		tpipePA = tpipePA&0xfffff000;
		struct dip_imem_memory dip_p2GCEdump_imem_buf;

		struct ion_client *dip_p2GCEdump_ion_client;

		dip_p2GCEdump_imem_buf.handle = NULL;
		dip_p2GCEdump_imem_buf.ion_fd = 0;
		dip_p2GCEdump_imem_buf.va = 0;
		dip_p2GCEdump_imem_buf.pa = 0;
		dip_p2GCEdump_imem_buf.length = TPIPE_DUMP_SIZE;
		if ((dip_p2_ion_client == NULL) && (g_ion_device))
			dip_p2_ion_client = ion_client_create(g_ion_device, "dip_p2");
		if (dip_p2_ion_client == NULL)
			LOG_ERR("invalid dip_p2_ion_client client!\n");
		if (dip_allocbuf(&dip_p2GCEdump_imem_buf) >= 0) {
			pMapVa = (int *)dip_p2GCEdump_imem_buf.va;
		LOG_DBG("ctlStart(0x%x),tpipePA(0x%x)", ctlStart, tpipePA);

		if (pMapVa) {
			for (i = 0; i < TPIPE_DUMP_SIZE; i += 10) {
				LOG_DBG("[idx(%d)]%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X",
					i, pMapVa[i], pMapVa[i+1], pMapVa[i+2], pMapVa[i+3], pMapVa[i+4],
					pMapVa[i+5], pMapVa[i+6], pMapVa[i+7], pMapVa[i+8], pMapVa[i+9]);
			}
		}
			dip_freebuf(&dip_p2GCEdump_imem_buf);
			dip_p2GCEdump_imem_buf.handle = NULL;
			dip_p2GCEdump_imem_buf.ion_fd = 0;
			dip_p2GCEdump_imem_buf.va = 0;
			dip_p2GCEdump_imem_buf.pa = 0;
		}
#endif
	}
#endif

	return 0;
}

/* #define _debug_dma_err_ */
#if defined(_debug_dma_err_)
#define bit(x) (0x1<<(x))

unsigned int DMA_ERR[3 * 12] = {
	bit(1), 0xF50043A8, 0x00000011, /* IMGI */
	bit(2), 0xF50043AC, 0x00000021, /* IMGCI */
	bit(4), 0xF50043B0, 0x00000031, /* LSCI */
	bit(5), 0xF50043B4, 0x00000051, /* FLKI */
	bit(6), 0xF50043B8, 0x00000061, /* LCEI */
	bit(7), 0xF50043BC, 0x00000071, /* VIPI */
	bit(8), 0xF50043C0, 0x00000081, /* VIP2I */
	bit(9), 0xF50043C4, 0x00000194, /* IMGO */
	bit(10), 0xF50043C8, 0x000001a4, /* IMG2O */
	bit(11), 0xF50043CC, 0x000001b4, /* LCSO */
	bit(12), 0xF50043D0, 0x000001c4, /* ESFKO */
	bit(13), 0xF50043D4, 0x000001d4, /* AAO */
};

static signed int DMAErrHandler(void)
{
	unsigned int err_ctrl = DIP_RD32(0xF50043A4);

	LOG_DBG("err_ctrl(0x%08x)", err_ctrl);

	unsigned int i = 0;

	unsigned int *pErr = DMA_ERR;

	for (i = 0; i < 12; i++) {
		unsigned int addr = 0;

#if 1
		if (err_ctrl & (*pErr)) {
			DIP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, DIP_RD32(addr),
				DIP_RD32(0xF5004160), DIP_RD32(0xF5004164));
		}
#else
		addr = pErr[1];
		unsigned int status = DIP_RD32(addr);

		if (status & 0x0000FFFF) {
			DIP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, status,
				DIP_RD32(0xF5004160), DIP_RD32(0xF5004164));
		}
#endif
		pErr = pErr + 3;
	}

}
#endif


void DIP_IRQ_INT_ERR_CHECK_CAM(unsigned int WarnStatus, unsigned int ErrStatus, enum DIP_IRQ_TYPE_ENUM module)
{
	/* ERR print */
	/* unsigned int i = 0; */
	if (ErrStatus) {
		switch (module) {
		case DIP_IRQ_TYPE_INT_DIP_A_ST:
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"DIP_A:int_err:0x%x_0x%x\n", WarnStatus, ErrStatus);
			break;
		default:
			break;
		}
		/*SMI monitor*/
/* ALSK TBD */
		/*
		* if (WarnStatus & (RRZO_ERR_ST|AFO_ERR_ST|IMGO_ERR_ST|AAO_ERR_ST|LCSO_ERR_ST|BNR_ERR_ST|LSC_ERR_ST)) {
		*	for (i = 0 ; i < 5 ; i++)
		*		smi_dumpDebugMsg();
		* }
		*/
	}
}

irqreturn_t DIP_Irq_DIP_A(signed int  Irq, void *DeviceId)
{
	int i = 0;
	unsigned int IrqINTStatus = 0x0;
	unsigned int IrqCQStatus = 0x0;
	unsigned int IrqCQLDStatus = 0x0;

	/*LOG_DBG("DIP_Irq_DIP_A:%d\n", Irq);*/

	/* Avoid touch hwmodule when clock is disable. DEVAPC will moniter this kind of err */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	spin_lock(&(IspInfo.SpinLockIrq[DIP_IRQ_TYPE_INT_DIP_A_ST]));
	IrqINTStatus = DIP_RD32(DIP_DIP_A_BASE + 0x030); /* DIP_A_REG_CTL_INT_STATUS */
	IrqCQStatus = DIP_RD32(DIP_DIP_A_BASE + 0x034); /* DIP_A_REG_CTL_CQ_INT_STATUS */
	IrqCQLDStatus = DIP_RD32(DIP_DIP_A_BASE + 0x038);

	for (i = 0; i < IRQ_USER_NUM_MAX; i++)
		IspInfo.IrqInfo.Status[DIP_IRQ_TYPE_INT_DIP_A_ST][i] |= IrqINTStatus;

	spin_unlock(&(IspInfo.SpinLockIrq[DIP_IRQ_TYPE_INT_DIP_A_ST]));

	/*LOG_DBG("DIP_Irq_DIP_A:%d, reg 0x%p : 0x%x, reg 0x%p : 0x%x\n",
	*	Irq, (DIP_DIP_A_BASE + 0x030), IrqINTStatus, (DIP_DIP_A_BASE + 0x034), IrqCQStatus);
	*/

	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[DIP_IRQ_TYPE_INT_DIP_A_ST]);

	return IRQ_HANDLED;

}

/*******************************************************************************
*
********************************************************************************/

#if (DIP_BOTTOMHALF_WORKQ == 1)
static void DIP_BH_Workqueue(struct work_struct *pWork)
{
	struct IspWorkqueTable *pWorkTable = container_of(pWork, struct IspWorkqueTable, dip_bh_work);

	IRQ_LOG_PRINTER(pWorkTable->module, m_CurrentPPB, _LOG_ERR);
	IRQ_LOG_PRINTER(pWorkTable->module, m_CurrentPPB, _LOG_INF);
}
#endif

/*******************************************************************************
*
********************************************************************************/
module_init(DIP_Init);
module_exit(DIP_Exit);
MODULE_DESCRIPTION("Camera DIP driver");
MODULE_AUTHOR("MM3/SW5");
MODULE_LICENSE("GPL");
