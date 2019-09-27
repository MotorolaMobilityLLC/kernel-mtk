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
 * camera_isp.c - MT6763 Linux ISP Device Driver
 *
 * DESCRIPTION:
 *     This file provid the other drivers ISP relative functions
 *
 ******************************************************************************/
/* MET: define to enable MET*/
/* #define ISP_MET_READY */

#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>  /* proc file use */
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
/*for kernel log reduction*/
#include <linux/printk.h>

#include <mt-plat/sync_write.h> /* For mt65xx_reg_sync_writel(). */

#ifdef COFNIG_MTK_IOMMU
#include <mtk_iommu.h>
#else
#include <m4u.h>
#endif

/*#define EP_CODE_MARK_CMDQ*/
#ifndef EP_CODE_MARK_CMDQ
#include <cmdq_core.h>
#endif

#include <smi_public.h>
/*for SMI BW debug log*/
#include <smi_debug.h>

/*for kernel log count*/
#define _K_LOG_ADJUST (1)

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
#else
#include <linux/wakelock.h>
#endif

#ifdef CONFIG_OF
#include <linux/of_platform.h>  /* for device tree */
#include <linux/of_irq.h>       /* for device tree */
#include <linux/of_address.h>   /* for device tree */
#endif

#if defined(ISP_MET_READY)
/*MET:met mmsys profile*/
#include <mt-plat/met_drv.h>
#endif

#include <mmdvfs_mgr.h>
/* Use this qos request to control camera dynamic frequency change */
struct mmdvfs_pm_qos_request isp_qos;
static unsigned int target_clk;

#define CREATE_TRACE_POINTS
#include "inc/met_events_camsys.h"

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

#include "inc/camera_isp.h"

/*  */
#ifndef MTRUE
#define MTRUE               1
#endif
#ifndef MFALSE
#define MFALSE              0
#endif

#define ISP_DEV_NAME                "camera-isp"
#define SMI_LARB_MMU_CTL            (1)

/*for early if load dont need to use camera*/
#define DUMMY_INT                   (0)

/* Clkmgr is not ready in early porting, en/disable clock  by hardcode */
/* #define EP_NO_CLKMGR */
/*#define ENABLE_WAITIRQ_LOG*/ /* wait irq debug logs */
/*#define ENABLE_STT_IRQ_LOG*/  /*show STT irq debug logs */
/* Queue timestamp for deque. Update when non-drop frame @SOF */
#define TIMESTAMP_QUEUE_EN          (0)
#if (TIMESTAMP_QUEUE_EN == 1)
#define TSTMP_SUBSAMPLE_INTPL		(1)
#else
#define TSTMP_SUBSAMPLE_INTPL		(0)
#endif
#define ISP_BOTTOMHALF_WORKQ		(1)

#if (ISP_BOTTOMHALF_WORKQ == 1)
#include <linux/workqueue.h>
#endif

/* -------------------------------------------------------------------------- */

#define MyTag "[ISP]"
#define IRQTag "KEEPER"

#define LOG_VRB(format, args...) \
	pr_debug(MyTag "[%s] " format, __func__, ##args)

#define ISP_DEBUG
#ifdef ISP_DEBUG
#define LOG_DBG(format, args...) \
	pr_info(MyTag "[%s] " format, __func__, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...) \
	pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_NOTICE(format, args...) \
	pr_notice(MyTag "[%s] " format, __func__, ##args)
#define LOG_WRN(format, args...) \
	pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_ERR(format, args...) \
	pr_info(MyTag "[%s] " format, __func__, ##args)
#define LOG_AST(format, args...) \
	pr_info(MyTag "[%s] " format, __func__, ##args)

/*******************************************************************************
 *
 ******************************************************************************/
/* For 89     Only.*/   /* NEED_TUNING_BY_PROJECT */
#define ISP_WR32(addr, data)    mt_reg_sync_writel(data, addr)
#define ISP_RD32(addr)                  ioread32((void *)addr)
/*******************************************************************************
 *
 ******************************************************************************/
/* dynamic log level */
#define ISP_DBG_INT                 (0x00000001)
#define ISP_DBG_READ_REG            (0x00000004)
#define ISP_DBG_WRITE_REG           (0x00000008)
#define ISP_DBG_CLK                 (0x00000010)
#define ISP_DBG_TASKLET             (0x00000020)
#define ISP_DBG_SCHEDULE_WORK       (0x00000040)
#define ISP_DBG_BUF_WRITE           (0x00000080)
#define ISP_DBG_BUF_CTRL            (0x00000100)
#define ISP_DBG_REF_CNT_CTRL        (0x00000200)
#define ISP_DBG_INT_2               (0x00000400)
#define ISP_DBG_INT_3               (0x00000800)
#define ISP_DBG_HW_DON              (0x00001000)
#define ISP_DBG_ION_CTRL            (0x00002000)
/*******************************************************************************
 *
 ******************************************************************************/
#define DUMP_GCE_TPIPE  0


/**
 *    CAM interrupt status
 */
/* normal siganl */
#define VS_INT_ST           (1L<<0)
#define TG_INT1_ST          (1L<<1)
#define TG_INT2_ST          (1L<<2)
#define EXPDON_ST           (1L<<3)
#define HW_PASS1_DON_ST     (1L<<11)
#define SOF_INT_ST          (1L<<12)
#define SW_PASS1_DON_ST     (1L<<30)
/* err status */
#define TG_ERR_ST           (1L<<4)
#define TG_GBERR_ST         (1L<<5)
#define CQ_CODE_ERR_ST      (1L<<6)
#define CQ_APB_ERR_ST       (1L<<7)
#define CQ_VS_ERR_ST        (1L<<8)
#define AMX_ERR_ST          (1L<<15)
#define RMX_ERR_ST          (1L<<16)
#define BMX_ERR_ST          (1L<<17)
#define RRZO_ERR_ST         (1L<<18)
#define AFO_ERR_ST          (1L<<19)
#define IMGO_ERR_ST         (1L<<20)
#define AAO_ERR_ST          (1L<<21)
#define PSO_ERR_ST          (1L<<22)
#define LCSO_ERR_ST         (1L<<23)
#define BNR_ERR_ST          (1L<<24)
#define LSC_ERR_ST          (1L<<25)
#define CAC_ERR_ST          (1L<<26)
#define DMA_ERR_ST          (1L<<29)
/**
 *    CAM DMA done status
 */
#define IMGO_DONE_ST        (1L<<0)
#define UFEO_DONE_ST        (1L<<1)
#define RRZO_DONE_ST        (1L<<2)
#define EISO_DONE_ST        (1L<<3)
#define FLKO_DONE_ST        (1L<<4)
#define AFO_DONE_ST         (1L<<5)
#define LCSO_DONE_ST        (1L<<6)
#define AAO_DONE_ST         (1L<<7)
#define BPCI_DONE_ST        (1L<<9)
#define LSCI_DONE_ST        (1L<<10)
/* #define CACI_DONE_ST        (1L<<11) */
#define PDO_DONE_ST         (1L<<13)
#define PSO_DONE_ST         (1L<<14)
/**
 *    CAMSV interrupt status
 */
/* normal signal */
#define SV_VS1_ST           (1L<<0)
#define SV_TG_ST1           (1L<<1)
#define SV_TG_ST2           (1L<<2)
#define SV_EXPDON_ST        (1L<<3)
#define SV_SOF_INT_ST       (1L<<7)
#define SV_HW_PASS1_DON_ST  (1L<<10)
#define SV_SW_PASS1_DON_ST  (1L<<20)
/* err status */
#define SV_TG_ERR           (1L<<4)
#define SV_TG_GBERR         (1L<<5)
#define SV_IMGO_ERR         (1L<<16)
#define SV_IMGO_OVERRUN     (1L<<17)

/**
 *    IRQ signal mask
 */
#define INT_ST_MASK_CAM     ( \
			      VS_INT_ST |\
			      TG_INT1_ST |\
			      TG_INT2_ST |\
			      EXPDON_ST |\
			      HW_PASS1_DON_ST |\
			      SOF_INT_ST |\
			      SW_PASS1_DON_ST)
/**
 *    dma done mask
 */
#define DMA_ST_MASK_CAM     (\
			     IMGO_DONE_ST |\
			     UFEO_DONE_ST |\
			     RRZO_DONE_ST |\
			     EISO_DONE_ST |\
			     FLKO_DONE_ST |\
			     AFO_DONE_ST |\
			     LCSO_DONE_ST |\
			     AAO_DONE_ST |\
			     BPCI_DONE_ST |\
			     LSCI_DONE_ST |\
			     PDO_DONE_ST | \
			     PSO_DONE_ST)

/**
 *    IRQ Warning Mask
 */
#define INT_ST_MASK_CAM_WARN    (\
				 RRZO_ERR_ST |\
				 AFO_ERR_ST |\
				 IMGO_ERR_ST |\
				 AAO_ERR_ST |\
				 PSO_ERR_ST | \
				 LCSO_ERR_ST |\
				 BNR_ERR_ST |\
				 LSC_ERR_ST)

/**
 *    IRQ Error Mask
 */
#define INT_ST_MASK_CAM_ERR     (\
				 TG_ERR_ST |\
				 TG_GBERR_ST |\
				 CQ_CODE_ERR_ST |\
				 CQ_APB_ERR_ST |\
				 CQ_VS_ERR_ST |\
				 AMX_ERR_ST |\
				 RMX_ERR_ST |\
				 BMX_ERR_ST |\
				 BNR_ERR_ST |\
				 LSC_ERR_ST |\
				 DMA_ERR_ST)


/**
 *    IRQ signal mask
 */
#define INT_ST_MASK_CAMSV       (\
				 SV_VS1_ST |\
				 SV_TG_ST1 |\
				 SV_TG_ST2 |\
				 SV_EXPDON_ST |\
				 SV_SOF_INT_ST |\
				 SV_HW_PASS1_DON_ST |\
				 SV_SW_PASS1_DON_ST)
/**
 *    IRQ Error Mask
 */
#define INT_ST_MASK_CAMSV_ERR   (\
				 SV_TG_ERR |\
				 SV_TG_GBERR |\
				 SV_IMGO_ERR |\
				 SV_IMGO_OVERRUN)

static irqreturn_t ISP_Irq_CAM_A(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAM_B(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAMSV_0(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAMSV_1(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAMSV_2(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAMSV_3(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAMSV_4(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_CAMSV_5(int  Irq, void *DeviceId);
static irqreturn_t ISP_Irq_DIP_A(int  Irq, void *DeviceId);


typedef irqreturn_t (*IRQ_CB)(int, void *);

struct ISR_TABLE {
	IRQ_CB          isr_fp;
	unsigned int    int_number;
	char            device_name[16];
};

#ifndef CONFIG_OF
const struct ISR_TABLE IRQ_CB_TBL[ISP_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_CAM_A,     CAM0_IRQ_BIT_ID,    "CAM_A"},
	{NULL,                            0,    "CAM_B"},
	{NULL,                            0,    "DIP_A"},
	{ISP_Irq_CAMSV_0,   CAM_SV0_IRQ_BIT_ID, "CAMSV_0"},
	{ISP_Irq_CAMSV_1,   CAM_SV1_IRQ_BIT_ID, "CAMSV_1"},
	{NULL,                               0,     "UNI"}
};

#else
/* int number is got from kernel api */

const struct ISR_TABLE IRQ_CB_TBL[ISP_IRQ_TYPE_AMOUNT] = {
#if DUMMY_INT
	/* Must be the same name with that in device node. */
	{ISP_Irq_CAM_A,     0,  "cam2-dummy"},
	{ISP_Irq_CAM_B,     0,  "cam3-dummy"},
	{ISP_Irq_DIP_A,     0,  "dip-dummy"},
	{ISP_Irq_CAMSV_0,   0,  "camsv1-dummy"},
	{ISP_Irq_CAMSV_1,   0,  "camsv2-dummy"},
	{ISP_Irq_CAMSV_2,   0,  "camsv3-dummy"},
	{ISP_Irq_CAMSV_3,   0,  "camsv4-dummy"},
	{ISP_Irq_CAMSV_4,   0,  "camsv5-dummy"},
	{ISP_Irq_CAMSV_5,   0,  "camsv6-dummy"},
	{NULL,              0,  "cam1-dummy"} /* UNI */
#else
	/* Must be the same name with that in device node. */
	{ISP_Irq_CAM_A,     0,  "cam2"},
	{ISP_Irq_CAM_B,     0,  "cam3"},
	{ISP_Irq_DIP_A,     0,  "dip"},
	{ISP_Irq_CAMSV_0,   0,  "camsv1"},
	{ISP_Irq_CAMSV_1,   0,  "camsv2"},
	{ISP_Irq_CAMSV_2,   0,  "camsv3"},
	{ISP_Irq_CAMSV_3,   0,  "camsv4"},
	{ISP_Irq_CAMSV_4,   0,  "camsv5"},
	{ISP_Irq_CAMSV_5,   0,  "camsv6"},
	{NULL,              0,  "cam1"} /* UNI */
#endif
};

/*
 * Note!!! The order and member of .compatible must be the same with that in
 *  "ISP_DEV_NODE_ENUM" in camera_isp.h
 */
static const struct of_device_id isp_of_ids[] = {
	{ .compatible = "mediatek,imgsys", },
	/* Remider: Add this device node manually in .dtsi */
	{ .compatible = "mediatek,dip1", },
	{ .compatible = "mediatek,camsys", },
	{ .compatible = "mediatek,cam1", },
	{ .compatible = "mediatek,cam2", },
	{ .compatible = "mediatek,cam3", },
	{ .compatible = "mediatek,camsv1", },
	{ .compatible = "mediatek,camsv2", },
	{ .compatible = "mediatek,camsv3", },
	{ .compatible = "mediatek,camsv4", },
	{ .compatible = "mediatek,camsv5", },
	{ .compatible = "mediatek,camsv6", },
	{}
};

#endif
/* ////////////////////////////////////////////////////////////////////////// */
/*  */
typedef void (*tasklet_cb)(unsigned long);
struct Tasklet_table {
	tasklet_cb tkt_cb;
	struct tasklet_struct  *pIsp_tkt;
};

struct tasklet_struct tkt[ISP_IRQ_TYPE_AMOUNT];

static void ISP_TaskletFunc_CAM_A(unsigned long data);
static void ISP_TaskletFunc_CAM_B(unsigned long data);
static void ISP_TaskletFunc_SV_0(unsigned long data);
static void ISP_TaskletFunc_SV_1(unsigned long data);
static void ISP_TaskletFunc_SV_2(unsigned long data);
static void ISP_TaskletFunc_SV_3(unsigned long data);
static void ISP_TaskletFunc_SV_4(unsigned long data);
static void ISP_TaskletFunc_SV_5(unsigned long data);

static struct Tasklet_table isp_tasklet[ISP_IRQ_TYPE_AMOUNT] = {
	{ISP_TaskletFunc_CAM_A, &tkt[ISP_IRQ_TYPE_INT_CAM_A_ST]},
	{ISP_TaskletFunc_CAM_B, &tkt[ISP_IRQ_TYPE_INT_CAM_B_ST]},
	{NULL,                  &tkt[ISP_IRQ_TYPE_INT_DIP_A_ST]},
	{ISP_TaskletFunc_SV_0,  &tkt[ISP_IRQ_TYPE_INT_CAMSV_0_ST]},
	{ISP_TaskletFunc_SV_1,  &tkt[ISP_IRQ_TYPE_INT_CAMSV_1_ST]},
	{ISP_TaskletFunc_SV_2,  &tkt[ISP_IRQ_TYPE_INT_CAMSV_2_ST]},
	{ISP_TaskletFunc_SV_3,  &tkt[ISP_IRQ_TYPE_INT_CAMSV_3_ST]},
	{ISP_TaskletFunc_SV_4,  &tkt[ISP_IRQ_TYPE_INT_CAMSV_4_ST]},
	{ISP_TaskletFunc_SV_5,  &tkt[ISP_IRQ_TYPE_INT_CAMSV_5_ST]},
	{NULL,                  &tkt[ISP_IRQ_TYPE_INT_UNI_A_ST]}
};

#if (ISP_BOTTOMHALF_WORKQ == 1)
struct IspWorkqueTable {
	enum ISP_IRQ_TYPE_ENUM	module;
	struct work_struct  isp_bh_work;
};

static void ISP_BH_Workqueue(struct work_struct *pWork);

static struct IspWorkqueTable isp_workque[ISP_IRQ_TYPE_AMOUNT] = {
	{ISP_IRQ_TYPE_INT_CAM_A_ST},
	{ISP_IRQ_TYPE_INT_CAM_B_ST},
	{ISP_IRQ_TYPE_INT_DIP_A_ST},
	{ISP_IRQ_TYPE_INT_CAMSV_0_ST},
	{ISP_IRQ_TYPE_INT_CAMSV_1_ST},
	{ISP_IRQ_TYPE_INT_CAMSV_2_ST},
	{ISP_IRQ_TYPE_INT_CAMSV_3_ST},
	{ISP_IRQ_TYPE_INT_CAMSV_4_ST},
	{ISP_IRQ_TYPE_INT_CAMSV_5_ST},
	{ISP_IRQ_TYPE_INT_UNI_A_ST}
};
#endif


#ifdef CONFIG_OF

#ifndef CONFIG_MTK_CLKMGR /*CCF*/
#include <linux/clk.h>
struct ISP_CLK_STRUCT {
	struct clk *ISP_SCP_SYS_DIS;
	struct clk *ISP_SCP_SYS_ISP;
	struct clk *ISP_SCP_SYS_CAM;
	struct clk *ISP_IMG_DIP;
	struct clk *ISP_CAM_CAMSYS;
	struct clk *ISP_CAM_CAMTG;
	struct clk *ISP_CAM_SENINF;
	struct clk *ISP_CAM_CAMSV0;
	struct clk *ISP_CAM_CAMSV1;
	struct clk *ISP_CAM_CAMSV2;
};
struct ISP_CLK_STRUCT isp_clk;
#endif

/* TODO: Remove, Jessy */
/*static unsigned long gISPSYS_Irq[ISP_IRQ_TYPE_AMOUNT];*/
/*static unsigned long gISPSYS_Reg[ISP_CAM_BASEADDR_NUM];*/

#ifdef CONFIG_OF
struct isp_device {
	void __iomem *regs;
	struct device *dev;
	int irq;
};

static struct isp_device *isp_devs;
static int nr_isp_devs;
#endif

#define AEE_DUMP_BY_USING_ION_MEMORY
#define AEE_DUMP_REDUCE_MEMORY
#ifdef AEE_DUMP_REDUCE_MEMORY
/* ion */

static DEFINE_MUTEX(gDipMutex);
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
#include <ion.h>
#include <mtk/ion_drv.h>
#include <mtk/mtk_ion.h>

struct isp_imem_memory {
	void *handle;
	int ion_fd;
	uint64_t va;
	uint32_t pa;
	uint32_t length;
};

static struct ion_client *isp_p2_ion_client;
static struct isp_imem_memory g_isp_p2_imem_buf;
#endif
static bool g_bIonBufferAllocated;
static unsigned int *g_pPhyISPBuffer;
/* Kernel Warning */
static unsigned int *g_pKWTpipeBuffer;
static unsigned int *g_pKWCmdqBuffer;
static unsigned int *g_pKWVirISPBuffer;
/* Navtive Exception */
static unsigned int *g_pTuningBuffer;
static unsigned int *g_pTpipeBuffer;
static unsigned int *g_pVirISPBuffer;
static unsigned int *g_pCmdqBuffer;
#else
/* Kernel Warning */
static unsigned int g_KWTpipeBuffer[(MAX_ISP_TILE_TDR_HEX_NO >> 2)];
static unsigned int g_KWCmdqBuffer[(MAX_ISP_CMDQ_BUFFER_SIZE >> 2)];
static unsigned int g_KWVirISPBuffer[(ISP_DIP_REG_SIZE >> 2)];
/* Navtive Exception */
static unsigned int g_PhyISPBuffer[(ISP_DIP_REG_SIZE >> 2)];
static unsigned int g_TuningBuffer[(ISP_DIP_REG_SIZE >> 2)];
static unsigned int g_TpipeBuffer[(MAX_ISP_TILE_TDR_HEX_NO >> 2)];
static unsigned int g_VirISPBuffer[(ISP_DIP_REG_SIZE >> 2)];
static unsigned int g_CmdqBuffer[(MAX_ISP_CMDQ_BUFFER_SIZE >> 2)];
#endif
static bool g_bUserBufIsReady = MFALSE;
static unsigned int DumpBufferField;
static bool g_bDumpPhyISPBuf = MFALSE;
static unsigned int g_tdriaddr = 0xffffffff;
static unsigned int g_cmdqaddr = 0xffffffff;
static struct ISP_GET_DUMP_INFO_STRUCT g_dumpInfo = {
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static struct ISP_MEM_INFO_STRUCT g_TpipeBaseAddrInfo = {0x0, 0x0, NULL, 0x0};
static struct ISP_MEM_INFO_STRUCT g_CmdqBaseAddrInfo = {0x0, 0x0, NULL, 0x0};
static unsigned int m_CurrentPPB;

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source isp_wake_lock;
#else
struct wake_lock isp_wake_lock;
#endif
static int g_WaitLockCt;
/*
* static void __iomem *g_isp_base_dase;
* static void __iomem *g_isp_inner_base_dase;
* static void __iomem *g_imgsys_config_base_dase;
*/

/* Get HW modules' base address from device nodes */
#define ISP_CAMSYS_CONFIG_BASE          (isp_devs[ISP_CAMSYS_CONFIG_IDX].regs)
#define ISP_CAM_A_BASE                  (isp_devs[ISP_CAM_A_IDX].regs)
#define ISP_CAM_B_BASE                  (isp_devs[ISP_CAM_B_IDX].regs)
#define ISP_CAMSV0_BASE                 (isp_devs[ISP_CAMSV0_IDX].regs)
#define ISP_CAMSV1_BASE                 (isp_devs[ISP_CAMSV1_IDX].regs)
#define ISP_CAMSV2_BASE                 (isp_devs[ISP_CAMSV2_IDX].regs)
#define ISP_CAMSV3_BASE                 (isp_devs[ISP_CAMSV3_IDX].regs)
#define ISP_CAMSV4_BASE                 (isp_devs[ISP_CAMSV4_IDX].regs)
#define ISP_CAMSV5_BASE                 (isp_devs[ISP_CAMSV5_IDX].regs)
#define ISP_CAM_UNI_BASE                (isp_devs[ISP_UNI_A_IDX].regs)
#define ISP_IMGSYS_CONFIG_BASE          (isp_devs[ISP_IMGSYS_CONFIG_IDX].regs)
#define ISP_DIP_A_BASE                  (isp_devs[ISP_DIP_A_IDX].regs)

void __iomem *ISP_SENINF0_BASE;
void __iomem *ISP_SENINF1_BASE;
void __iomem *ISP_SENINF2_BASE;
void __iomem *ISP_SENINF3_BASE;

void __iomem *CLOCK_CELL_BASE;

void __iomem *ISP_MMSYS_CONFIG_BASE;

#if (SMI_LARB_MMU_CTL == 1)
void __iomem *SMI_LARB_BASE[8];
#endif


/* TODO: Remove start, Jessy */
#define ISP_ADDR                      (gISPSYS_Reg[ISP_BASE_ADDR])
#define ISP_IMGSYS_BASE               (gISPSYS_Reg[ISP_IMGSYS_CONFIG_BASE_ADDR])
#define ISP_ADDR_CAMINF               (gISPSYS_Reg[ISP_IMGSYS_CONFIG_BASE_ADDR])

#define ISP_MIPI_ANA_ADDR             (gISPSYS_Reg[ISP_MIPI_ANA_BASE_ADDR])
#define ISP_GPIO_ADDR                 (gISPSYS_Reg[ISP_GPIO_BASE_ADDR])

#define ISP_IMGSYS_BASE_PHY             0x15000000

#else
#define ISP_ADDR                        (IMGSYS_BASE + 0x4000)
#define ISP_IMGSYS_BASE                 IMGSYS_BASE
#define ISP_ADDR_CAMINF                 IMGSYS_BASE
#define ISP_MIPI_ANA_ADDR               0x10217000
#define ISP_GPIO_ADDR                   GPIO_BASE

#endif
/* TODO: Remove end, Jessy */


#define ISP_REG_SW_CTL_RST_CAM_P1       (1)
#define ISP_REG_SW_CTL_RST_CAM_P2       (2)
#define ISP_REG_SW_CTL_RST_CAMSV        (3)
#define ISP_REG_SW_CTL_RST_CAMSV2       (4)

struct S_START_T {
	unsigned int sec;
	unsigned int usec;
};

/* QQ, remove later */
/* record remain node count(success/fail) excludes head,
 * when enque/deque control.
 */
static unsigned int g_regScen = 0xa5a5a5a5; /* remove later */


static /*volatile*/ wait_queue_head_t P2WaitQueueHead_WaitDeque;
static /*volatile*/ wait_queue_head_t P2WaitQueueHead_WaitFrame;
static /*volatile*/ wait_queue_head_t P2WaitQueueHead_WaitFrameEQDforDQ;
static spinlock_t      SpinLock_P2FrameList;
#define _MAX_SUPPORT_P2_FRAME_NUM_ 512
#define _MAX_SUPPORT_P2_BURSTQ_NUM_ 8
#define _MAX_SUPPORT_P2_PACKAGE_NUM_ \
	(_MAX_SUPPORT_P2_FRAME_NUM_/_MAX_SUPPORT_P2_BURSTQ_NUM_)
struct ISP_P2_BUFQUE_IDX_STRUCT {
	/*volatile */int start; /* starting index for frames in the ring list */

	/* current index for running frame in the ring list */
	/*volatile */int curr;
	/*volatile */int end; /* ending index for frames in the ring list */
};

struct ISP_P2_FRAME_UNIT_STRUCT {
	/*volatile */unsigned int          processID;  /* caller process ID */
	/*volatile */unsigned int          callerID;   /* caller thread ID */

	/* QQ. optional -> to judge cq combination(for judging frame) */
	/*volatile */unsigned int          cqMask;

	/*volatile */enum ISP_P2_BUF_STATE_ENUM  bufSts;     /* buffer status */
};

static /*volatile */struct ISP_P2_BUFQUE_IDX_STRUCT
	P2_FrameUnit_List_Idx[ISP_P2_BUFQUE_PROPERTY_NUM];
static /*volatile */struct ISP_P2_FRAME_UNIT_STRUCT
P2_FrameUnit_List[ISP_P2_BUFQUE_PROPERTY_NUM][_MAX_SUPPORT_P2_FRAME_NUM_];

struct ISP_P2_FRAME_PACKAGE_STRUCT {
	/*volatile */unsigned int           processID;  /* caller process ID */
	/*volatile */unsigned int           callerID;   /* caller thread ID */

	/* to judge it belongs to which frame package */
	/*volatile */unsigned int           dupCQIdx;
	/*volatile */int                   frameNum;

	/* number of dequed buffer no matter deque success or fail */
	/*volatile */int                   dequedNum;
};
static /*volatile */struct ISP_P2_BUFQUE_IDX_STRUCT
	P2_FramePack_List_Idx[ISP_P2_BUFQUE_PROPERTY_NUM];
static /*volatile */struct ISP_P2_FRAME_PACKAGE_STRUCT
P2_FramePackage_List[ISP_P2_BUFQUE_PROPERTY_NUM][_MAX_SUPPORT_P2_PACKAGE_NUM_];




static  spinlock_t      SpinLockRegScen;

/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32
static  spinlock_t      SpinLock_UserKey;



#if (TIMESTAMP_QUEUE_EN == 1)
static int32_t ISP_PopBufTimestamp(unsigned int module, unsigned int dma_id,
	struct S_START_T *pTstp);
static int32_t ISP_WaitTimestampReady(unsigned int module, unsigned int dma_id);
#endif

/*******************************************************************************
 *
 ******************************************************************************/
/* internal data */
/* pointer to the kmalloc'd area, rounded up to a page boundary */
static int *pTbl_RTBuf[ISP_IRQ_TYPE_AMOUNT];
static int Tbl_RTBuf_MMPSize[ISP_IRQ_TYPE_AMOUNT];

/* original pointer for kmalloc'd area as returned by kmalloc */
static void *pBuf_kmalloc[ISP_IRQ_TYPE_AMOUNT];
/*  */
static struct ISP_RT_BUF_STRUCT *pstRTBuf[ISP_IRQ_TYPE_AMOUNT] = {NULL};

unsigned long g_Flash_SpinLock;


static unsigned int G_u4EnableClockCount;
static atomic_t G_u4DevNodeCt;

int pr_detect_count;

/*save ion fd*/
#define ENABLE_KEEP_ION_HANDLE

#ifdef ENABLE_KEEP_ION_HANDLE
#define _ion_keep_max_   (64)/*32*/
#include "ion_drv.h" /*g_ion_device*/
static struct ion_client *pIon_client;
static int G_WRDMA_IonCt[2][_dma_max_wr_*_ion_keep_max_] = { {0}, {0} };
static int G_WRDMA_IonFd[2][_dma_max_wr_*_ion_keep_max_] = { {0}, {0} };
static struct ion_handle *G_WRDMA_IonHnd[2][_dma_max_wr_*_ion_keep_max_] = {
	{NULL}, {NULL} };

/* protect G_WRDMA_IonHnd & G_WRDMA_IonFd */
static spinlock_t SpinLock_IonHnd[2][_dma_max_wr_];

struct T_ION_TBL {
	enum ISP_DEV_NODE_ENUM node;
	int *pIonCt;
	int *pIonFd;
	struct ion_handle **pIonHnd;
	spinlock_t *pLock;
};

static struct T_ION_TBL gION_TBL[ISP_DEV_NODE_NUM] = {
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_CAM_A_IDX, (int *)G_WRDMA_IonCt[0], (int *)G_WRDMA_IonFd[0],
		(struct ion_handle **)G_WRDMA_IonHnd[0],
		(spinlock_t *)SpinLock_IonHnd[0]},
	{ISP_CAM_B_IDX, (int *)G_WRDMA_IonCt[1], (int *)G_WRDMA_IonFd[1],
		(struct ion_handle **)G_WRDMA_IonHnd[1],
		(spinlock_t *)SpinLock_IonHnd[1]},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL},
	{ISP_DEV_NODE_NUM, NULL, NULL, NULL, NULL}
};
#endif
/*******************************************************************************
 *
 ******************************************************************************/
struct ISP_USER_INFO_STRUCT {
	pid_t   Pid;
	pid_t   Tid;
};

/*******************************************************************************
 *
 ******************************************************************************/
#define ISP_BUF_SIZE            (4096)
#define ISP_BUF_SIZE_WRITE      1024
#define ISP_BUF_WRITE_AMOUNT    6

enum ISP_BUF_STATUS_ENUM {
	ISP_BUF_STATUS_EMPTY,
	ISP_BUF_STATUS_HOLD,
	ISP_BUF_STATUS_READY
};

struct ISP_BUF_STRUCT {
	enum ISP_BUF_STATUS_ENUM Status;
	unsigned int                Size;
	unsigned char *pData;
};

struct ISP_BUF_INFO_STRUCT {
	struct ISP_BUF_STRUCT      Read;
	struct ISP_BUF_STRUCT      Write[ISP_BUF_WRITE_AMOUNT];
};


/*******************************************************************************
 *
 ******************************************************************************/
#define ISP_ISR_MAX_NUM 32
#define INT_ERR_WARN_TIMER_THREAS 1000
#define INT_ERR_WARN_MAX_TIME 1

struct ISP_IRQ_ERR_WAN_CNT_STRUCT {
	/* cnt for each err int # */
	unsigned int m_err_int_cnt[ISP_IRQ_TYPE_AMOUNT][ISP_ISR_MAX_NUM];

	/* cnt for each warning int # */
	unsigned int m_warn_int_cnt[ISP_IRQ_TYPE_AMOUNT][ISP_ISR_MAX_NUM];

	/* mark for err int, where its cnt > threshold */
	unsigned int m_err_int_mark[ISP_IRQ_TYPE_AMOUNT];

	/* mark for warn int, where its cnt > threshold */
	unsigned int m_warn_int_mark[ISP_IRQ_TYPE_AMOUNT];
	unsigned long m_int_usec[ISP_IRQ_TYPE_AMOUNT];
};

static int FirstUnusedIrqUserKey = 1;
#define USERKEY_STR_LEN 128

struct UserKeyInfo {
	/* name for the user that register a userKey */
	char userName[USERKEY_STR_LEN];
	int userKey;    /* the user key for that user */
};
/* array for recording the user name for a specific user key */
static struct UserKeyInfo IrqUserKey_UserInfo[IRQ_USER_NUM_MAX];

struct ISP_IRQ_INFO_STRUCT {
	/* Add an extra index for status type in mt6797 -> signal or dma */
	unsigned int    Status[ISP_IRQ_TYPE_AMOUNT][ISP_IRQ_ST_AMOUNT]
			      [IRQ_USER_NUM_MAX];
	unsigned int Mask[ISP_IRQ_TYPE_AMOUNT][ISP_IRQ_ST_AMOUNT];
	unsigned int ErrMask[ISP_IRQ_TYPE_AMOUNT][ISP_IRQ_ST_AMOUNT];
	unsigned int WarnMask[ISP_IRQ_TYPE_AMOUNT][ISP_IRQ_ST_AMOUNT];
	/* flag for indicating that user do mark for a interrupt or not */
	unsigned int    MarkedFlag[ISP_IRQ_TYPE_AMOUNT][ISP_IRQ_ST_AMOUNT]
				  [IRQ_USER_NUM_MAX];
	/* time for marking a specific interrupt */
	unsigned int    MarkedTime_sec[ISP_IRQ_TYPE_AMOUNT][32]
				      [IRQ_USER_NUM_MAX];
	/* time for marking a specific interrupt */
	unsigned int    MarkedTime_usec[ISP_IRQ_TYPE_AMOUNT][32]
				       [IRQ_USER_NUM_MAX];
	/* number of a specific signal that passed by */
	int             PassedBySigCnt[ISP_IRQ_TYPE_AMOUNT][32]
				      [IRQ_USER_NUM_MAX];
	/* */
	unsigned int    LastestSigTime_sec[ISP_IRQ_TYPE_AMOUNT][32];
	/* latest time for each interrupt */
	unsigned int    LastestSigTime_usec[ISP_IRQ_TYPE_AMOUNT][32];
	/* latest time for each interrupt */
};

struct ISP_TIME_LOG_STRUCT {
	unsigned int     Vd;
	unsigned int     Expdone;
	unsigned int     WorkQueueVd;
	unsigned int     WorkQueueExpdone;
	unsigned int     TaskletVd;
	unsigned int     TaskletExpdone;
};

#if (TIMESTAMP_QUEUE_EN == 1)
#define ISP_TIMESTPQ_DEPTH      (256)
struct ISP_TIMESTPQ_INFO_STRUCT {
	struct {
		struct S_START_T   TimeQue[ISP_TIMESTPQ_DEPTH];
		/* increase when p1done or dmao done */
		unsigned int     WrIndex;
		unsigned int     RdIndex; /* increase when user deque */
		unsigned long long  TotalWrCnt;
		unsigned long long  TotalRdCnt;
		/* TSTP_V3 unsigned int	    PrevFbcDropCnt; */
		unsigned int     PrevFbcWCnt;
	} Dmao[_cam_max_];
	unsigned int  DmaEnStatus[_cam_max_];
};
#endif

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


static unsigned int g_ISPIntErr[ISP_IRQ_TYPE_AMOUNT] = {0};
static unsigned int g_DmaErr_CAM[ISP_IRQ_TYPE_AMOUNT][_cam_max_] = {{0} };



#define SUPPORT_MAX_IRQ 32
struct ISP_INFO_STRUCT {
	spinlock_t                      SpinLockIspRef;
	spinlock_t                      SpinLockIsp;
	spinlock_t                      SpinLockIrq[ISP_IRQ_TYPE_AMOUNT];
	spinlock_t                      SpinLockIrqCnt[ISP_IRQ_TYPE_AMOUNT];
	spinlock_t                      SpinLockRTBC;
	spinlock_t                      SpinLockClock;
	wait_queue_head_t               WaitQueueHead[ISP_IRQ_TYPE_AMOUNT];
	/* wait_queue_head_t*              WaitQHeadList; */
	wait_queue_head_t      WaitQHeadList[SUPPORT_MAX_IRQ];
	unsigned int                         UserCount;
	unsigned int                         DebugMask;
	int							IrqNum;
	struct ISP_IRQ_INFO_STRUCT			IrqInfo;
	struct ISP_IRQ_ERR_WAN_CNT_STRUCT		IrqCntInfo;
	struct ISP_BUF_INFO_STRUCT			BufInfo;
	struct ISP_TIME_LOG_STRUCT             TimeLog;
	#if (TIMESTAMP_QUEUE_EN == 1)
	struct ISP_TIMESTPQ_INFO_STRUCT        TstpQInfo[ISP_IRQ_TYPE_AMOUNT];
	#endif
};

static struct ISP_INFO_STRUCT IspInfo;
static bool    SuspnedRecord[ISP_DEV_NODE_NUM] = {0};

enum eLOG_TYPE {
	/* currently, only used at ipl_buf_ctrl. to protect critical section */
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
struct SV_LOG_STR {
	unsigned int _cnt[LOG_PPNUM][_LOG_MAX];
	/* char   _str[_LOG_MAX][SV_LOG_STR_LEN]; */
	char *_str[LOG_PPNUM][_LOG_MAX];
	struct S_START_T   _lastIrqTime;
};

static void *pLog_kmalloc;
static struct SV_LOG_STR gSvLog[ISP_IRQ_TYPE_AMOUNT];

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
	int avaLen;\
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
	ptr = pDes = (char *)&(gSvLog[irq].\
		_str[ppb][logT][gSvLog[irq]._cnt[ppb][logT]]);\
	\
	avaLen = str_leng - 1 - gSvLog[irq]._cnt[ppb][logT];\
	if (avaLen > 1) {\
		snprintf((char *)(pDes), avaLen, "[%d.%06d]" fmt,\
			gSvLog[irq]._lastIrqTime.sec,\
			gSvLog[irq]._lastIrqTime.usec,\
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
					if (ptr[NORMAL_STR_LEN*(i+1) - 1] != \
					    '\0') {\
						ptr[NORMAL_STR_LEN*(i+1) - 1] =\
							'\0';\
						LOG_DBG("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
					} else{\
						LOG_DBG("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
						break;\
					} \
				} \
			} \
			else if (logT == _LOG_INF) {\
				for (i = 0; i < INF_PAGE; i++) {\
					if (ptr[NORMAL_STR_LEN*(i+1) - 1] != \
					    '\0') {\
						ptr[NORMAL_STR_LEN*(i+1) - 1] =\
						    '\0';\
						LOG_INF("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
					} else{\
						LOG_INF("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
						break;\
					} \
				} \
			} \
			else if (logT == _LOG_ERR) {\
				for (i = 0; i < ERR_PAGE; i++) {\
					if (ptr[NORMAL_STR_LEN*(i+1) - 1] != \
					    '\0') {\
						ptr[NORMAL_STR_LEN*(i+1) - 1] =\
							'\0';\
						LOG_ERR("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
					} else{\
						LOG_ERR("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
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
			ptr = pDes = (char *)&(pSrc->\
				_str[ppb][logT][pSrc->_cnt[ppb][logT]]);\
			\
			ptr2 = &(pSrc->_cnt[ppb][logT]);\
			snprintf((char *)(pDes), avaLen, fmt, ##__VA_ARGS__);\
			while (*ptr++ != '\0') {\
				(*ptr2)++;\
			} \
		} \
	} \
} while (0)
#else
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, args...) \
		pr_info(IRQTag fmt,  ##args)
#endif

#if 1
#define IRQ_LOG_PRINTER(irq, ppb_in, logT_in) do {\
		struct SV_LOG_STR *pSrc = &gSvLog[irq];\
		char *ptr;\
		unsigned int i;\
		int ppb = 0;\
		int logT = 0;\
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
					if (ptr[NORMAL_STR_LEN*(i+1) - 1] !=\
					    '\0') {\
						ptr[NORMAL_STR_LEN*(i+1) - 1] =\
						'\0';\
						LOG_DBG("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
					} else{\
						LOG_DBG("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
						break;\
					} \
				} \
			} \
			else if (logT == _LOG_INF) {\
				for (i = 0; i < INF_PAGE; i++) {\
					if (ptr[NORMAL_STR_LEN*(i+1) - 1] !=\
					    '\0') {\
						ptr[NORMAL_STR_LEN*(i+1) - 1] =\
							'\0';\
						\
						LOG_INF("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
					} else{\
						LOG_INF("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
						break;\
					} \
				} \
			} \
			else if (logT == _LOG_ERR) {\
				for (i = 0; i < ERR_PAGE; i++) {\
					if (ptr[NORMAL_STR_LEN*(i+1) - 1] !=\
						'\0') {\
						ptr[NORMAL_STR_LEN*(i+1) - 1] =\
						'\0';\
						LOG_NOTICE("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
					} else{\
						LOG_NOTICE("%s",\
						    &ptr[NORMAL_STR_LEN*i]);\
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
union FBC_CTRL_1 {
	struct { /* 0x18004110 */
		unsigned int  FBC_NUM        :  6;      /*  0.. 5, 0x0000003F */
		unsigned int  rsv_6          :  9;      /*  6..14, 0x00007FC0 */
		unsigned int  FBC_EN         :  1;      /* 15..15, 0x00008000 */
		unsigned int  FBC_MODE       :  1;      /* 16..16, 0x00010000 */
		unsigned int  LOCK_EN        :  1;      /* 17..17, 0x00020000 */
		unsigned int  rsv_18         :  2;      /* 18..19, 0x000C0000 */
		unsigned int  DROP_TIMING    :  1;      /* 20..20, 0x00100000 */
		unsigned int  rsv_21         :  3;      /* 21..23, 0x00E00000 */
		unsigned int  SUB_RATIO      :  8;      /* 24..31, 0xFF000000 */
	} Bits;
	unsigned int Raw;
};  /* CAM_A_FBC_IMGO_CTL1 */

union FBC_CTRL_2 {
	struct { /* 0x18004114 */
		unsigned int  FBC_CNT        :  7;      /*  0.. 6, 0x0000007F */
		unsigned int  rsv_7          :  1;      /*  7.. 7, 0x00000080 */
		unsigned int  RCNT           :  6;      /*  8..13, 0x00003F00 */
		unsigned int  rsv_14         :  2;      /* 14..15, 0x0000C000 */
		unsigned int  WCNT           :  6;      /* 16..21, 0x003F0000 */
		unsigned int  rsv_22         :  2;      /* 22..23, 0x00C00000 */
		unsigned int  DROP_CNT       :  8;      /* 24..31, 0xFF000000 */
	} Bits;
	unsigned int Raw;
};  /* CAM_A_FBC_IMGO_CTL2 */


struct _isp_bk_reg_t {
	unsigned int  CAM_TG_INTER_ST;                                 /* 453C*/
};

static struct _isp_bk_reg_t g_BkReg[ISP_IRQ_TYPE_AMOUNT];

/* mt6797 top registers */
#define CAMSYS_REG_CG_CON               (ISP_CAMSYS_CONFIG_BASE + 0x0)
#define IMGSYS_REG_CG_CON               (ISP_IMGSYS_CONFIG_BASE + 0x0)
#define CAMSYS_REG_CG_SET               (ISP_CAMSYS_CONFIG_BASE + 0x4)
#define IMGSYS_REG_CG_SET               (ISP_IMGSYS_CONFIG_BASE + 0x4)
#define CAMSYS_REG_CG_CLR               (ISP_CAMSYS_CONFIG_BASE + 0x8)
#define IMGSYS_REG_CG_CLR               (ISP_IMGSYS_CONFIG_BASE + 0x8)

/* mt6797 CAM registers */
#define CAM_REG_CTL_START(module)  (isp_devs[module].regs + 0x0000)
#define CAM_REG_CTL_EN(module)  (isp_devs[module].regs + 0x0004)
#define CAM_REG_CTL_DMA_EN(module)  (isp_devs[module].regs + 0x0008)
#define CAM_REG_CTL_FMT_SEL(module)  (isp_devs[module].regs + 0x000C)
#define CAM_REG_CTL_SEL(module)  (isp_devs[module].regs + 0x0010)
#define CAM_REG_CTL_MISC(module)  (isp_devs[module].regs + 0x0014)
#define CAM_REG_CTL_RAW_INT_EN(module)  (isp_devs[module].regs + 0x0020)
#define CAM_REG_CTL_RAW_INT_STATUS(module)  (isp_devs[module].regs + 0x0024)
#define CAM_REG_CTL_RAW_INT_STATUSX(module)  (isp_devs[module].regs + 0x0028)
#define CAM_REG_CTL_RAW_INT2_EN(module)  (isp_devs[module].regs + 0x0030)
#define CAM_REG_CTL_RAW_INT2_STATUS(module)  (isp_devs[module].regs + 0x0034)
#define CAM_REG_CTL_RAW_INT2_STATUSX(module)  (isp_devs[module].regs + 0x0038)
#define CAM_REG_CTL_SW_CTL(module)  (isp_devs[module].regs + 0x0040)
#define CAM_REG_CTL_AB_DONE_SEL(module)  (isp_devs[module].regs + 0x0044)
#define CAM_REG_CTL_CD_DONE_SEL(module)  (isp_devs[module].regs + 0x0048)
#define CAM_REG_CTL_UNI_DONE_SEL(module)  (isp_devs[module].regs + 0x004C)
#define CAM_REG_CTL_TWIN_STATUS(module)  (isp_devs[module].regs + 0x0050)
#define CAM_REG_CTL_SPARE1(module)  (isp_devs[module].regs + 0x0054)
#define CAM_REG_CTL_SPARE2(module)  (isp_devs[module].regs + 0x0058)
#define CAM_REG_CTL_SW_PASS1_DONE(module)  (isp_devs[module].regs + 0x005C)
#define CAM_REG_CTL_FBC_RCNT_INC(module)  (isp_devs[module].regs + 0x0060)
#define CAM_REG_CTL_DBG_SET(module)  (isp_devs[module].regs + 0x0070)
#define CAM_REG_CTL_DBG_PORT(module)  (isp_devs[module].regs + 0x0074)
#define CAM_REG_CTL_DATE_CODE(module)  (isp_devs[module].regs + 0x0078)
#define CAM_REG_CTL_PROJ_CODE(module)  (isp_devs[module].regs + 0x007C)
#define CAM_REG_CTL_RAW_DCM_DIS(module)  (isp_devs[module].regs + 0x0080)
#define CAM_REG_CTL_DMA_DCM_DIS(module)  (isp_devs[module].regs + 0x0084)
#define CAM_REG_CTL_TOP_DCM_DIS(module)  (isp_devs[module].regs + 0x0088)
#define CAM_REG_CTL_RAW_DCM_STATUS(module)  (isp_devs[module].regs + 0x0090)
#define CAM_REG_CTL_DMA_DCM_STATUS(module)  (isp_devs[module].regs + 0x0094)
#define CAM_REG_CTL_TOP_DCM_STATUS(module)  (isp_devs[module].regs + 0x0098)
#define CAM_REG_CTL_RAW_REQ_STATUS(module)  (isp_devs[module].regs + 0x00A0)
#define CAM_REG_CTL_DMA_REQ_STATUS(module)  (isp_devs[module].regs + 0x00A4)
#define CAM_REG_CTL_RAW_RDY_STATUS(module)  (isp_devs[module].regs + 0x00A8)
#define CAM_REG_CTL_DMA_RDY_STATUS(module)  (isp_devs[module].regs + 0x00AC)
#define CAM_REG_FBC_IMGO_CTL1(module)  (isp_devs[module].regs + 0x0110)
#define CAM_REG_FBC_IMGO_CTL2(module)  (isp_devs[module].regs + 0x0114)
#define CAM_REG_FBC_RRZO_CTL1(module)  (isp_devs[module].regs + 0x0118)
#define CAM_REG_FBC_RRZO_CTL2(module)  (isp_devs[module].regs + 0x011C)
#define CAM_REG_FBC_UFEO_CTL1(module)  (isp_devs[module].regs + 0x0120)
#define CAM_REG_FBC_UFEO_CTL2(module)  (isp_devs[module].regs + 0x0124)
#define CAM_REG_FBC_LCSO_CTL1(module)  (isp_devs[module].regs + 0x0128)
#define CAM_REG_FBC_LCSO_CTL2(module)  (isp_devs[module].regs + 0x012C)
#define CAM_REG_FBC_AFO_CTL1(module)  (isp_devs[module].regs + 0x0130)
#define CAM_REG_FBC_AFO_CTL2(module)  (isp_devs[module].regs + 0x0134)
#define CAM_REG_FBC_AAO_CTL1(module)  (isp_devs[module].regs + 0x0138)
#define CAM_REG_FBC_AAO_CTL2(module)  (isp_devs[module].regs + 0x013C)
#define CAM_REG_FBC_PDO_CTL1(module)  (isp_devs[module].regs + 0x0140)
#define CAM_REG_FBC_PDO_CTL2(module)  (isp_devs[module].regs + 0x0144)
#define CAM_REG_FBC_PSO_CTL1(module)  (isp_devs[module].regs + 0x0148)
#define CAM_REG_FBC_PSO_CTL2(module)  (isp_devs[module].regs + 0x014C)
#define CAM_REG_CQ_EN(module)  (isp_devs[module].regs + 0x0160)
#define CAM_REG_CQ_THR0_CTL(module)  (isp_devs[module].regs + 0x0164)
#define CAM_REG_CQ_THR0_BASEADDR(module)  (isp_devs[module].regs + 0x0168)
#define CAM_REG_CQ_THR0_DESC_SIZE(module)  (isp_devs[module].regs + 0x016C)
#define CAM_REG_CQ_THR1_CTL(module)  (isp_devs[module].regs + 0x0170)
#define CAM_REG_CQ_THR1_BASEADDR(module)  (isp_devs[module].regs + 0x0174)
#define CAM_REG_CQ_THR1_DESC_SIZE(module)  (isp_devs[module].regs + 0x0178)
#define CAM_REG_CQ_THR2_CTL(module)  (isp_devs[module].regs + 0x017C)
#define CAM_REG_CQ_THR2_BASEADDR(module)  (isp_devs[module].regs + 0x0180)
#define CAM_REG_CQ_THR2_DESC_SIZE(module)  (isp_devs[module].regs + 0x0184)
#define CAM_REG_CQ_THR3_CTL(module)  (isp_devs[module].regs + 0x0188)
#define CAM_REG_CQ_THR3_BASEADDR(module)  (isp_devs[module].regs + 0x018C)
#define CAM_REG_CQ_THR3_DESC_SIZE(module)  (isp_devs[module].regs + 0x0190)
#define CAM_REG_CQ_THR4_CTL(module)  (isp_devs[module].regs + 0x0194)
#define CAM_REG_CQ_THR4_BASEADDR(module)  (isp_devs[module].regs + 0x0198)
#define CAM_REG_CQ_THR4_DESC_SIZE(module)  (isp_devs[module].regs + 0x019C)
#define CAM_REG_CQ_THR5_CTL(module)  (isp_devs[module].regs + 0x01A0)
#define CAM_REG_CQ_THR5_BASEADDR(module)  (isp_devs[module].regs + 0x01A4)
#define CAM_REG_CQ_THR5_DESC_SIZE(module)  (isp_devs[module].regs + 0x01A8)
#define CAM_REG_CQ_THR6_CTL(module)  (isp_devs[module].regs + 0x01AC)
#define CAM_REG_CQ_THR6_BASEADDR(module)  (isp_devs[module].regs + 0x01B0)
#define CAM_REG_CQ_THR6_DESC_SIZE(module)  (isp_devs[module].regs + 0x01B4)
#define CAM_REG_CQ_THR7_CTL(module)  (isp_devs[module].regs + 0x01B8)
#define CAM_REG_CQ_THR7_BASEADDR(module)  (isp_devs[module].regs + 0x01BC)
#define CAM_REG_CQ_THR7_DESC_SIZE(module)  (isp_devs[module].regs + 0x01C0)
#define CAM_REG_CQ_THR8_CTL(module)  (isp_devs[module].regs + 0x01C4)
#define CAM_REG_CQ_THR8_BASEADDR(module)  (isp_devs[module].regs + 0x01C8)
#define CAM_REG_CQ_THR8_DESC_SIZE(module)  (isp_devs[module].regs + 0x01CC)
#define CAM_REG_CQ_THR9_CTL(module)  (isp_devs[module].regs + 0x01D0)
#define CAM_REG_CQ_THR9_BASEADDR(module)  (isp_devs[module].regs + 0x01D4)
#define CAM_REG_CQ_THR9_DESC_SIZE(module)  (isp_devs[module].regs + 0x01D8)
#define CAM_REG_CQ_THR10_CTL(module)  (isp_devs[module].regs + 0x01DC)
#define CAM_REG_CQ_THR10_BASEADDR(module)  (isp_devs[module].regs + 0x01E0)
#define CAM_REG_CQ_THR10_DESC_SIZE(module)  (isp_devs[module].regs + 0x01E4)
#define CAM_REG_CQ_THR11_CTL(module)  (isp_devs[module].regs + 0x01E8)
#define CAM_REG_CQ_THR11_BASEADDR(module)  (isp_devs[module].regs + 0x01EC)
#define CAM_REG_CQ_THR11_DESC_SIZE(module)  (isp_devs[module].regs + 0x01F0)
#define CAM_REG_CQ_THR12_CTL(module)  (isp_devs[module].regs + 0x01F4)
#define CAM_REG_CQ_THR12_BASEADDR(module)  (isp_devs[module].regs + 0x01F8)
#define CAM_REG_CQ_THR12_DESC_SIZE(module)  (isp_devs[module].regs + 0x01FC)
#define CAM_REG_DMA_SOFT_RSTSTAT(module)  (isp_devs[module].regs + 0x0200)
#define CAM_REG_CQ0I_BASE_ADDR(module)  (isp_devs[module].regs + 0x0204)
#define CAM_REG_CQ0I_XSIZE(module)  (isp_devs[module].regs + 0x0208)
#define CAM_REG_VERTICAL_FLIP_EN(module)  (isp_devs[module].regs + 0x020C)
#define CAM_REG_DMA_SOFT_RESET(module)  (isp_devs[module].regs + 0x0210)
#define CAM_REG_LAST_ULTRA_EN(module)  (isp_devs[module].regs + 0x0214)
#define CAM_REG_SPECIAL_FUN_EN(module)  (isp_devs[module].regs + 0x0218)
#define CAM_REG_IMGO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0220)
#define CAM_REG_IMGO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0228)
#define CAM_REG_IMGO_XSIZE(module)  (isp_devs[module].regs + 0x0230)
#define CAM_REG_IMGO_YSIZE(module)  (isp_devs[module].regs + 0x0234)
#define CAM_REG_IMGO_STRIDE(module)  (isp_devs[module].regs + 0x0238)
#define CAM_REG_IMGO_CON(module)  (isp_devs[module].regs + 0x023C)
#define CAM_REG_IMGO_CON2(module)  (isp_devs[module].regs + 0x0240)
#define CAM_REG_IMGO_CON3(module)  (isp_devs[module].regs + 0x0244)
#define CAM_REG_IMGO_CROP(module)  (isp_devs[module].regs + 0x0248)
#define CAM_REG_RRZO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0250)
#define CAM_REG_RRZO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0258)
#define CAM_REG_RRZO_XSIZE(module)  (isp_devs[module].regs + 0x0260)
#define CAM_REG_RRZO_YSIZE(module)  (isp_devs[module].regs + 0x0264)
#define CAM_REG_RRZO_STRIDE(module)  (isp_devs[module].regs + 0x0268)
#define CAM_REG_RRZO_CON(module)  (isp_devs[module].regs + 0x026C)
#define CAM_REG_RRZO_CON2(module)  (isp_devs[module].regs + 0x0270)
#define CAM_REG_RRZO_CON3(module)  (isp_devs[module].regs + 0x0274)
#define CAM_REG_RRZO_CROP(module)  (isp_devs[module].regs + 0x0278)
#define CAM_REG_AAO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0280)
#define CAM_REG_AAO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0288)
#define CAM_REG_AAO_XSIZE(module)  (isp_devs[module].regs + 0x0290)
#define CAM_REG_AAO_YSIZE(module)  (isp_devs[module].regs + 0x0294)
#define CAM_REG_AAO_STRIDE(module)  (isp_devs[module].regs + 0x0298)
#define CAM_REG_AAO_CON(module)  (isp_devs[module].regs + 0x029C)
#define CAM_REG_AAO_CON2(module)  (isp_devs[module].regs + 0x02A0)
#define CAM_REG_AAO_CON3(module)  (isp_devs[module].regs + 0x02A4)
#define CAM_REG_AFO_BASE_ADDR(module)  (isp_devs[module].regs + 0x02B0)
#define CAM_REG_AFO_OFST_ADDR(module)  (isp_devs[module].regs + 0x02B8)
#define CAM_REG_AFO_XSIZE(module)  (isp_devs[module].regs + 0x02C0)
#define CAM_REG_AFO_YSIZE(module)  (isp_devs[module].regs + 0x02C4)
#define CAM_REG_AFO_STRIDE(module)  (isp_devs[module].regs + 0x02C8)
#define CAM_REG_AFO_CON(module)  (isp_devs[module].regs + 0x02CC)
#define CAM_REG_AFO_CON2(module)  (isp_devs[module].regs + 0x02D0)
#define CAM_REG_AFO_CON3(module)  (isp_devs[module].regs + 0x02D4)
#define CAM_REG_LCSO_BASE_ADDR(module)  (isp_devs[module].regs + 0x02E0)
#define CAM_REG_LCSO_OFST_ADDR(module)  (isp_devs[module].regs + 0x02E8)
#define CAM_REG_LCSO_XSIZE(module)  (isp_devs[module].regs + 0x02F0)
#define CAM_REG_LCSO_YSIZE(module)  (isp_devs[module].regs + 0x02F4)
#define CAM_REG_LCSO_STRIDE(module)  (isp_devs[module].regs + 0x02F8)
#define CAM_REG_LCSO_CON(module)  (isp_devs[module].regs + 0x02FC)
#define CAM_REG_LCSO_CON2(module)  (isp_devs[module].regs + 0x0300)
#define CAM_REG_LCSO_CON3(module)  (isp_devs[module].regs + 0x0304)
#define CAM_REG_UFEO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0310)
#define CAM_REG_UFEO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0318)
#define CAM_REG_UFEO_XSIZE(module)  (isp_devs[module].regs + 0x0320)
#define CAM_REG_UFEO_YSIZE(module)  (isp_devs[module].regs + 0x0324)
#define CAM_REG_UFEO_STRIDE(module)  (isp_devs[module].regs + 0x0328)
#define CAM_REG_UFEO_CON(module)  (isp_devs[module].regs + 0x032C)
#define CAM_REG_UFEO_CON2(module)  (isp_devs[module].regs + 0x0330)
#define CAM_REG_UFEO_CON3(module)  (isp_devs[module].regs + 0x0334)
#define CAM_REG_PDO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0340)
#define CAM_REG_PDO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0348)
#define CAM_REG_PDO_XSIZE(module)  (isp_devs[module].regs + 0x0350)
#define CAM_REG_PDO_YSIZE(module)  (isp_devs[module].regs + 0x0354)
#define CAM_REG_PDO_STRIDE(module)  (isp_devs[module].regs + 0x0358)
#define CAM_REG_PDO_CON(module)  (isp_devs[module].regs + 0x035C)
#define CAM_REG_PDO_CON2(module)  (isp_devs[module].regs + 0x0360)
#define CAM_REG_PDO_CON3(module)  (isp_devs[module].regs + 0x0364)
#define CAM_REG_BPCI_BASE_ADDR(module)  (isp_devs[module].regs + 0x0370)
#define CAM_REG_BPCI_OFST_ADDR(module)  (isp_devs[module].regs + 0x0378)
#define CAM_REG_BPCI_XSIZE(module)  (isp_devs[module].regs + 0x0380)
#define CAM_REG_BPCI_YSIZE(module)  (isp_devs[module].regs + 0x0384)
#define CAM_REG_BPCI_STRIDE(module)  (isp_devs[module].regs + 0x0388)
#define CAM_REG_BPCI_CON(module)  (isp_devs[module].regs + 0x038C)
#define CAM_REG_BPCI_CON2(module)  (isp_devs[module].regs + 0x0390)
#define CAM_REG_BPCI_CON3(module)  (isp_devs[module].regs + 0x0394)
#define CAM_REG_CACI_BASE_ADDR(module)  (isp_devs[module].regs + 0x03A0)
#define CAM_REG_CACI_OFST_ADDR(module)  (isp_devs[module].regs + 0x03A8)
#define CAM_REG_CACI_XSIZE(module)  (isp_devs[module].regs + 0x03B0)
#define CAM_REG_CACI_YSIZE(module)  (isp_devs[module].regs + 0x03B4)
#define CAM_REG_CACI_STRIDE(module)  (isp_devs[module].regs + 0x03B8)
#define CAM_REG_CACI_CON(module)  (isp_devs[module].regs + 0x03BC)
#define CAM_REG_CACI_CON2(module)  (isp_devs[module].regs + 0x03C0)
#define CAM_REG_CACI_CON3(module)  (isp_devs[module].regs + 0x03C4)
#define CAM_REG_LSCI_BASE_ADDR(module)  (isp_devs[module].regs + 0x03D0)
#define CAM_REG_LSCI_OFST_ADDR(module)  (isp_devs[module].regs + 0x03D8)
#define CAM_REG_LSCI_XSIZE(module)  (isp_devs[module].regs + 0x03E0)
#define CAM_REG_LSCI_YSIZE(module)  (isp_devs[module].regs + 0x03E4)
#define CAM_REG_LSCI_STRIDE(module)  (isp_devs[module].regs + 0x03E8)
#define CAM_REG_LSCI_CON(module)  (isp_devs[module].regs + 0x03EC)
#define CAM_REG_LSCI_CON2(module)  (isp_devs[module].regs + 0x03F0)
#define CAM_REG_LSCI_CON3(module)  (isp_devs[module].regs + 0x03F4)
#define CAM_REG_LSC3I_BASE_ADDR(module)  (isp_devs[module].regs + 0x0400)
#define CAM_REG_LSC3I_OFST_ADDR(module)  (isp_devs[module].regs + 0x0408)
#define CAM_REG_LSC3I_XSIZE(module)  (isp_devs[module].regs + 0x0410)
#define CAM_REG_LSC3I_YSIZE(module)  (isp_devs[module].regs + 0x0414)
#define CAM_REG_LSC3I_STRIDE(module)  (isp_devs[module].regs + 0x0418)
#define CAM_REG_LSC3I_CON(module)  (isp_devs[module].regs + 0x041C)
#define CAM_REG_LSC3I_CON2(module)  (isp_devs[module].regs + 0x0420)
#define CAM_REG_LSC3I_CON3(module)  (isp_devs[module].regs + 0x0424)
#define CAM_REG_DMA_ERR_CTRL(module)  (isp_devs[module].regs + 0x0440)
#define CAM_REG_IMGO_ERR_STAT(module)  (isp_devs[module].regs + 0x0444)
#define CAM_REG_RRZO_ERR_STAT(module)  (isp_devs[module].regs + 0x0448)
#define CAM_REG_AAO_ERR_STAT(module)  (isp_devs[module].regs + 0x044C)
#define CAM_REG_AFO_ERR_STAT(module)  (isp_devs[module].regs + 0x0450)
#define CAM_REG_LCSO_ERR_STAT(module)  (isp_devs[module].regs + 0x0454)
#define CAM_REG_UFEO_ERR_STAT(module)  (isp_devs[module].regs + 0x0458)
#define CAM_REG_PDO_ERR_STAT(module)  (isp_devs[module].regs + 0x045C)
#define CAM_REG_BPCI_ERR_STAT(module)  (isp_devs[module].regs + 0x0460)
#define CAM_REG_CACI_ERR_STAT(module)  (isp_devs[module].regs + 0x0464)
#define CAM_REG_LSCI_ERR_STAT(module)  (isp_devs[module].regs + 0x0468)
#define CAM_REG_LSC3I_ERR_STAT(module)  (isp_devs[module].regs + 0x046C)
#define CAM_REG_DMA_DEBUG_ADDR(module)  (isp_devs[module].regs + 0x0470)
#define CAM_REG_PSO_ERR_STAT(module)  (isp_devs[module].regs + 0x0DAC)
#define CAM_REG_DMA_RSV1(module)  (isp_devs[module].regs + 0x0470)
#define CAM_REG_DMA_RSV2(module)  (isp_devs[module].regs + 0x0474)
#define CAM_REG_DMA_RSV3(module)  (isp_devs[module].regs + 0x0478)
#define CAM_REG_DMA_RSV4(module)  (isp_devs[module].regs + 0x047C)
#define CAM_REG_DMA_RSV5(module)  (isp_devs[module].regs + 0x0480)
#define CAM_REG_DMA_RSV6(module)  (isp_devs[module].regs + 0x0484)
#define CAM_REG_DMA_DEBUG_SEL(module)  (isp_devs[module].regs + 0x0488)
#define CAM_REG_DMA_BW_SELF_TEST(module)  (isp_devs[module].regs + 0x048C)
#define CAM_REG_TG_SEN_MODE(module)  (isp_devs[module].regs + 0x0500)
#define CAM_REG_TG_VF_CON(module)  (isp_devs[module].regs + 0x0504)
#define CAM_REG_TG_SEN_GRAB_PXL(module)  (isp_devs[module].regs + 0x0508)
#define CAM_REG_TG_SEN_GRAB_LIN(module)  (isp_devs[module].regs + 0x050C)
#define CAM_REG_TG_PATH_CFG(module)  (isp_devs[module].regs + 0x0510)
#define CAM_REG_TG_MEMIN_CTL(module)  (isp_devs[module].regs + 0x0514)
#define CAM_REG_TG_INT1(module)  (isp_devs[module].regs + 0x0518)
#define CAM_REG_TG_INT2(module)  (isp_devs[module].regs + 0x051C)
#define CAM_REG_TG_SOF_CNT(module)  (isp_devs[module].regs + 0x0520)
#define CAM_REG_TG_SOT_CNT(module)  (isp_devs[module].regs + 0x0524)
#define CAM_REG_TG_EOT_CNT(module)  (isp_devs[module].regs + 0x0528)
#define CAM_REG_TG_ERR_CTL(module)  (isp_devs[module].regs + 0x052C)
#define CAM_REG_TG_DAT_NO(module)  (isp_devs[module].regs + 0x0530)
#define CAM_REG_TG_FRM_CNT_ST(module)  (isp_devs[module].regs + 0x0534)
#define CAM_REG_TG_FRMSIZE_ST(module)  (isp_devs[module].regs + 0x0538)
#define CAM_REG_TG_INTER_ST(module)  (isp_devs[module].regs + 0x053C)
#define CAM_REG_TG_FLASHA_CTL(module)  (isp_devs[module].regs + 0x0540)
#define CAM_REG_TG_FLASHA_LINE_CNT(module)  (isp_devs[module].regs + 0x0544)
#define CAM_REG_TG_FLASHA_POS(module)  (isp_devs[module].regs + 0x0548)
#define CAM_REG_TG_FLASHB_CTL(module)  (isp_devs[module].regs + 0x054C)
#define CAM_REG_TG_FLASHB_LINE_CNT(module)  (isp_devs[module].regs + 0x0550)
#define CAM_REG_TG_FLASHB_POS(module)  (isp_devs[module].regs + 0x0554)
#define CAM_REG_TG_FLASHB_POS1(module)  (isp_devs[module].regs + 0x0558)
#define CAM_REG_TG_I2C_CQ_TRIG(module)  (isp_devs[module].regs + 0x0560)
#define CAM_REG_TG_CQ_TIMING(module)  (isp_devs[module].regs + 0x0564)
#define CAM_REG_TG_CQ_NUM(module)  (isp_devs[module].regs + 0x0568)
#define CAM_REG_TG_TIME_STAMP(module)  (isp_devs[module].regs + 0x0570)
#define CAM_REG_TG_SUB_PERIOD(module)  (isp_devs[module].regs + 0x0574)
#define CAM_REG_TG_DAT_NO_R(module)  (isp_devs[module].regs + 0x0578)
#define CAM_REG_TG_FRMSIZE_ST_R(module)  (isp_devs[module].regs + 0x057C)
#define CAM_REG_DMX_CTL(module)  (isp_devs[module].regs + 0x0580)
#define CAM_REG_DMX_CROP(module)  (isp_devs[module].regs + 0x0584)
#define CAM_REG_DMX_VSIZE(module)  (isp_devs[module].regs + 0x0588)
#define CAM_REG_RMG_HDR_CFG(module)  (isp_devs[module].regs + 0x05A0)
#define CAM_REG_RMG_HDR_GAIN(module)  (isp_devs[module].regs + 0x05A4)
#define CAM_REG_RMG_HDR_CFG2(module)  (isp_devs[module].regs + 0x005A8)
#define CAM_REG_LCS25_CON(module)  (isp_devs[module].regs + 0x0880)
#define CAM_REG_LCS25_ST(module)  (isp_devs[module].regs + 0x0884)
#define CAM_REG_LCS25_AWS(module)  (isp_devs[module].regs + 0x0888)
#define CAM_REG_LCS25_FLR(module)  (isp_devs[module].regs + 0x088C)
#define CAM_REG_LCS25_LRZR_1(module)  (isp_devs[module].regs + 0x0890)
#define CAM_REG_LCS25_LRZR_2(module)  (isp_devs[module].regs + 0x0894)
#define CAM_REG_LCS25_SATU_1(module)  (isp_devs[module].regs + 0x0898)
#define CAM_REG_LCS25_SATU_2(module)  (isp_devs[module].regs + 0x089C)
#define CAM_REG_LCS25_GAIN_1(module)  (isp_devs[module].regs + 0x08A0)
#define CAM_REG_LCS25_GAIN_2(module)  (isp_devs[module].regs + 0x08A4)
#define CAM_REG_LCS25_OFST_1(module)  (isp_devs[module].regs + 0x08A8)
#define CAM_REG_LCS25_OFST_2(module)  (isp_devs[module].regs + 0x08AC)
#define CAM_REG_LCS25_G2G_CNV_1(module)  (isp_devs[module].regs + 0x08B0)
#define CAM_REG_LCS25_G2G_CNV_2(module)  (isp_devs[module].regs + 0x08B4)
#define CAM_REG_LCS25_G2G_CNV_3(module)  (isp_devs[module].regs + 0x08B8)
#define CAM_REG_LCS25_G2G_CNV_4(module)  (isp_devs[module].regs + 0x08BC)
#define CAM_REG_LCS25_G2G_CNV_5(module)  (isp_devs[module].regs + 0x08C0)
#define CAM_REG_LCS25_LPF(module)  (isp_devs[module].regs + 0x08C4)
#define CAM_REG_RMM_OSC(module)  (isp_devs[module].regs + 0x05C0)
#define CAM_REG_RMM_MC(module)  (isp_devs[module].regs + 0x05C4)
#define CAM_REG_RMM_REVG_1(module)  (isp_devs[module].regs + 0x05C8)
#define CAM_REG_RMM_REVG_2(module)  (isp_devs[module].regs + 0x05CC)
#define CAM_REG_RMM_LEOS(module)  (isp_devs[module].regs + 0x05D0)
#define CAM_REG_RMM_MC2(module)  (isp_devs[module].regs + 0x05D4)
#define CAM_REG_RMM_DIFF_LB(module)  (isp_devs[module].regs + 0x05D8)
#define CAM_REG_RMM_MA(module)  (isp_devs[module].regs + 0x05DC)
#define CAM_REG_RMM_TUNE(module)  (isp_devs[module].regs + 0x05E0)
#define CAM_REG_OBC_OFFST0(module)  (isp_devs[module].regs + 0x05F0)
#define CAM_REG_OBC_OFFST1(module)  (isp_devs[module].regs + 0x05F4)
#define CAM_REG_OBC_OFFST2(module)  (isp_devs[module].regs + 0x05F8)
#define CAM_REG_OBC_OFFST3(module)  (isp_devs[module].regs + 0x05FC)
#define CAM_REG_OBC_GAIN0(module)  (isp_devs[module].regs + 0x0600)
#define CAM_REG_OBC_GAIN1(module)  (isp_devs[module].regs + 0x0604)
#define CAM_REG_OBC_GAIN2(module)  (isp_devs[module].regs + 0x0608)
#define CAM_REG_OBC_GAIN3(module)  (isp_devs[module].regs + 0x060C)
#define CAM_REG_BNR_BPC_CON(module)  (isp_devs[module].regs + 0x0620)
#define CAM_REG_BNR_BPC_TH1(module)  (isp_devs[module].regs + 0x0624)
#define CAM_REG_BNR_BPC_TH2(module)  (isp_devs[module].regs + 0x0628)
#define CAM_REG_BNR_BPC_TH3(module)  (isp_devs[module].regs + 0x062C)
#define CAM_REG_BNR_BPC_TH4(module)  (isp_devs[module].regs + 0x0630)
#define CAM_REG_BNR_BPC_DTC(module)  (isp_devs[module].regs + 0x0634)
#define CAM_REG_BNR_BPC_COR(module)  (isp_devs[module].regs + 0x0638)
#define CAM_REG_BNR_BPC_TBLI1(module)  (isp_devs[module].regs + 0x063C)
#define CAM_REG_BNR_BPC_TBLI2(module)  (isp_devs[module].regs + 0x0640)
#define CAM_REG_BNR_BPC_TH1_C(module)  (isp_devs[module].regs + 0x0644)
#define CAM_REG_BNR_BPC_TH2_C(module)  (isp_devs[module].regs + 0x0648)
#define CAM_REG_BNR_BPC_TH3_C(module)  (isp_devs[module].regs + 0x064C)
#define CAM_REG_BNR_NR1_CON(module)  (isp_devs[module].regs + 0x0650)
#define CAM_REG_BNR_NR1_CT_CON(module)  (isp_devs[module].regs + 0x0654)
#define CAM_REG_BNR_RSV1(module)  (isp_devs[module].regs + 0x0658)
#define CAM_REG_BNR_RSV2(module)  (isp_devs[module].regs + 0x065C)
#define CAM_REG_BNR_PDC_CON(module)  (isp_devs[module].regs + 0x0660)
#define CAM_REG_BNR_PDC_GAIN_L0(module)  (isp_devs[module].regs + 0x0664)
#define CAM_REG_BNR_PDC_GAIN_L1(module)  (isp_devs[module].regs + 0x0668)
#define CAM_REG_BNR_PDC_GAIN_L2(module)  (isp_devs[module].regs + 0x066C)
#define CAM_REG_BNR_PDC_GAIN_L3(module)  (isp_devs[module].regs + 0x0670)
#define CAM_REG_BNR_PDC_GAIN_L4(module)  (isp_devs[module].regs + 0x0674)
#define CAM_REG_BNR_PDC_GAIN_R0(module)  (isp_devs[module].regs + 0x0678)
#define CAM_REG_BNR_PDC_GAIN_R1(module)  (isp_devs[module].regs + 0x067C)
#define CAM_REG_BNR_PDC_GAIN_R2(module)  (isp_devs[module].regs + 0x0680)
#define CAM_REG_BNR_PDC_GAIN_R3(module)  (isp_devs[module].regs + 0x0684)
#define CAM_REG_BNR_PDC_GAIN_R4(module)  (isp_devs[module].regs + 0x0688)
#define CAM_REG_BNR_PDC_TH_GB(module)  (isp_devs[module].regs + 0x068C)
#define CAM_REG_BNR_PDC_TH_IA(module)  (isp_devs[module].regs + 0x0690)
#define CAM_REG_BNR_PDC_TH_HD(module)  (isp_devs[module].regs + 0x0694)
#define CAM_REG_BNR_PDC_SL(module)  (isp_devs[module].regs + 0x0698)
#define CAM_REG_BNR_PDC_POS(module)  (isp_devs[module].regs + 0x069C)
#define CAM_REG_LSC_CTL1(module)  (isp_devs[module].regs + 0x06A0)
#define CAM_REG_LSC_CTL2(module)  (isp_devs[module].regs + 0x06A4)
#define CAM_REG_LSC_CTL3(module)  (isp_devs[module].regs + 0x06A8)
#define CAM_REG_LSC_LBLOCK(module)  (isp_devs[module].regs + 0x06AC)
#define CAM_REG_LSC_RATIO_0(module)  (isp_devs[module].regs + 0x07E0)
#define CAM_REG_LSC_RATIO(module)  (isp_devs[module].regs + 0x06B0)
#define CAM_REG_LSC_TPIPE_OFST(module)  (isp_devs[module].regs + 0x06B4)
#define CAM_REG_LSC_TPIPE_SIZE(module)  (isp_devs[module].regs + 0x06B8)
#define CAM_REG_LSC_GAIN_TH(module)  (isp_devs[module].regs + 0x06BC)
#define CAM_REG_LSC_RATIO_1(module)  (isp_devs[module].regs + 0x07F0)
#define CAM_REG_RPG_SATU_1(module)  (isp_devs[module].regs + 0x06C0)
#define CAM_REG_RPG_SATU_2(module)  (isp_devs[module].regs + 0x06C4)
#define CAM_REG_RPG_GAIN_1(module)  (isp_devs[module].regs + 0x06C8)
#define CAM_REG_RPG_GAIN_2(module)  (isp_devs[module].regs + 0x06CC)
#define CAM_REG_RPG_OFST_1(module)  (isp_devs[module].regs + 0x06D0)
#define CAM_REG_RPG_OFST_2(module)  (isp_devs[module].regs + 0x06D4)
#define CAM_REG_RRZ_CTL(module)  (isp_devs[module].regs + 0x06E0)
#define CAM_REG_RRZ_IN_IMG(module)  (isp_devs[module].regs + 0x06E4)
#define CAM_REG_RRZ_OUT_IMG(module)  (isp_devs[module].regs + 0x06E8)
#define CAM_REG_RRZ_HORI_STEP(module)  (isp_devs[module].regs + 0x06EC)
#define CAM_REG_RRZ_VERT_STEP(module)  (isp_devs[module].regs + 0x06F0)
#define CAM_REG_RRZ_HORI_INT_OFST(module)  (isp_devs[module].regs + 0x06F4)
#define CAM_REG_RRZ_HORI_SUB_OFST(module)  (isp_devs[module].regs + 0x06F8)
#define CAM_REG_RRZ_VERT_INT_OFST(module)  (isp_devs[module].regs + 0x06FC)
#define CAM_REG_RRZ_VERT_SUB_OFST(module)  (isp_devs[module].regs + 0x0700)
#define CAM_REG_RRZ_MODE_TH(module)  (isp_devs[module].regs + 0x0704)
#define CAM_REG_RRZ_MODE_CTL(module)  (isp_devs[module].regs + 0x0708)
#define CAM_REG_RMX_CTL(module)  (isp_devs[module].regs + 0x0740)
#define CAM_REG_RMX_CROP(module)  (isp_devs[module].regs + 0x0744)
#define CAM_REG_RMX_VSIZE(module)  (isp_devs[module].regs + 0x0748)
#define CAM_REG_SGG5_PGN(module)  (isp_devs[module].regs + 0x0760)
#define CAM_REG_SGG5_GMRC_1(module)  (isp_devs[module].regs + 0x0764)
#define CAM_REG_SGG5_GMRC_2(module)  (isp_devs[module].regs + 0x0768)
#define CAM_REG_BMX_CTL(module)  (isp_devs[module].regs + 0x0780)
#define CAM_REG_BMX_CROP(module)  (isp_devs[module].regs + 0x0784)
#define CAM_REG_BMX_VSIZE(module)  (isp_devs[module].regs + 0x0788)
#define CAM_REG_UFE_CON(module)  (isp_devs[module].regs + 0x07C0)
#define CAM_REG_LCS_CON(module)  (isp_devs[module].regs + 0x07E0)
#define CAM_REG_LCS_ST(module)  (isp_devs[module].regs + 0x07E4)
#define CAM_REG_LCS_AWS(module)  (isp_devs[module].regs + 0x07E8)
#define CAM_REG_LCS_FLR(module)  (isp_devs[module].regs + 0x07EC)
#define CAM_REG_LCS_LRZR_1(module)  (isp_devs[module].regs + 0x07F0)
#define CAM_REG_LCS_LRZR_2(module)  (isp_devs[module].regs + 0x07F4)
#define CAM_REG_AF_CON(module)  (isp_devs[module].regs + 0x0800)
#define CAM_REG_AF_TH_0(module)  (isp_devs[module].regs + 0x0804)
#define CAM_REG_AF_TH_1(module)  (isp_devs[module].regs + 0x0808)
#define CAM_REG_AF_FLT_1(module)  (isp_devs[module].regs + 0x080C)
#define CAM_REG_AF_FLT_2(module)  (isp_devs[module].regs + 0x0810)
#define CAM_REG_AF_FLT_3(module)  (isp_devs[module].regs + 0x0814)
#define CAM_REG_AF_FLT_4(module)  (isp_devs[module].regs + 0x0818)
#define CAM_REG_AF_FLT_5(module)  (isp_devs[module].regs + 0x081C)
#define CAM_REG_AF_FLT_6(module)  (isp_devs[module].regs + 0x0820)
#define CAM_REG_AF_FLT_7(module)  (isp_devs[module].regs + 0x0824)
#define CAM_REG_AF_FLT_8(module)  (isp_devs[module].regs + 0x0828)
#define CAM_REG_AF_SIZE(module)  (isp_devs[module].regs + 0x0830)
#define CAM_REG_AF_VLD(module)  (isp_devs[module].regs + 0x0834)
#define CAM_REG_AF_BLK_0(module)  (isp_devs[module].regs + 0x0838)
#define CAM_REG_AF_BLK_1(module)  (isp_devs[module].regs + 0x083C)
#define CAM_REG_AF_TH_2(module)  (isp_devs[module].regs + 0x0840)
#define CAM_REG_RCP_CROP_CON1(module)  (isp_devs[module].regs + 0x08F0)
#define CAM_REG_RCP_CROP_CON2(module)  (isp_devs[module].regs + 0x08F4)
#define CAM_REG_SGG1_PGN(module)  (isp_devs[module].regs + 0x0900)
#define CAM_REG_SGG1_GMRC_1(module)  (isp_devs[module].regs + 0x0904)
#define CAM_REG_SGG1_GMRC_2(module)  (isp_devs[module].regs + 0x0908)
#define CAM_REG_QBN2_MODE(module)  (isp_devs[module].regs + 0x0910)
#define CAM_REG_AWB_WIN_ORG(module)  (isp_devs[module].regs + 0x0920)
#define CAM_REG_AWB_WIN_SIZE(module)  (isp_devs[module].regs + 0x0924)
#define CAM_REG_AWB_WIN_PIT(module)  (isp_devs[module].regs + 0x0928)
#define CAM_REG_AWB_WIN_NUM(module)  (isp_devs[module].regs + 0x092C)
#define CAM_REG_AWB_GAIN1_0(module)  (isp_devs[module].regs + 0x0930)
#define CAM_REG_AWB_GAIN1_1(module)  (isp_devs[module].regs + 0x0934)
#define CAM_REG_AWB_LMT1_0(module)  (isp_devs[module].regs + 0x0938)
#define CAM_REG_AWB_LMT1_1(module)  (isp_devs[module].regs + 0x093C)
#define CAM_REG_AWB_LOW_THR(module)  (isp_devs[module].regs + 0x0940)
#define CAM_REG_AWB_HI_THR(module)  (isp_devs[module].regs + 0x0944)
#define CAM_REG_AWB_PIXEL_CNT0(module)  (isp_devs[module].regs + 0x0948)
#define CAM_REG_AWB_PIXEL_CNT1(module)  (isp_devs[module].regs + 0x094C)
#define CAM_REG_AWB_PIXEL_CNT2(module)  (isp_devs[module].regs + 0x0950)
#define CAM_REG_AWB_ERR_THR(module)  (isp_devs[module].regs + 0x0954)
#define CAM_REG_AWB_ROT(module)  (isp_devs[module].regs + 0x0958)
#define CAM_REG_AWB_L0_X(module)  (isp_devs[module].regs + 0x095C)
#define CAM_REG_AWB_L0_Y(module)  (isp_devs[module].regs + 0x0960)
#define CAM_REG_AWB_L1_X(module)  (isp_devs[module].regs + 0x0964)
#define CAM_REG_AWB_L1_Y(module)  (isp_devs[module].regs + 0x0968)
#define CAM_REG_AWB_L2_X(module)  (isp_devs[module].regs + 0x096C)
#define CAM_REG_AWB_L2_Y(module)  (isp_devs[module].regs + 0x0970)
#define CAM_REG_AWB_L3_X(module)  (isp_devs[module].regs + 0x0974)
#define CAM_REG_AWB_L3_Y(module)  (isp_devs[module].regs + 0x0978)
#define CAM_REG_AWB_L4_X(module)  (isp_devs[module].regs + 0x097C)
#define CAM_REG_AWB_L4_Y(module)  (isp_devs[module].regs + 0x0980)
#define CAM_REG_AWB_L5_X(module)  (isp_devs[module].regs + 0x0984)
#define CAM_REG_AWB_L5_Y(module)  (isp_devs[module].regs + 0x0988)
#define CAM_REG_AWB_L6_X(module)  (isp_devs[module].regs + 0x098C)
#define CAM_REG_AWB_L6_Y(module)  (isp_devs[module].regs + 0x0990)
#define CAM_REG_AWB_L7_X(module)  (isp_devs[module].regs + 0x0994)
#define CAM_REG_AWB_L7_Y(module)  (isp_devs[module].regs + 0x0998)
#define CAM_REG_AWB_L8_X(module)  (isp_devs[module].regs + 0x099C)
#define CAM_REG_AWB_L8_Y(module)  (isp_devs[module].regs + 0x09A0)
#define CAM_REG_AWB_L9_X(module)  (isp_devs[module].regs + 0x09A4)
#define CAM_REG_AWB_L9_Y(module)  (isp_devs[module].regs + 0x09A8)
#define CAM_REG_AWB_SPARE(module)  (isp_devs[module].regs + 0x09AC)
#define CAM_REG_AWB_MOTION_THR(module)  (isp_devs[module].regs + 0x09B0)
#define CAM_REG_AWB_RC_CNV_0(module)  (isp_devs[module].regs + 0x09B4)
#define CAM_REG_AWB_RC_CNV_1(module)  (isp_devs[module].regs + 0x09B8)
#define CAM_REG_AWB_RC_CNV_2(module)  (isp_devs[module].regs + 0x09BC)
#define CAM_REG_AWB_RC_CNV_3(module)  (isp_devs[module].regs + 0x09C0)
#define CAM_REG_AWB_RC_CNV_4(module)  (isp_devs[module].regs + 0x09C4)
#define CAM_REG_AE_HST_CTL(module)  (isp_devs[module].regs + 0x09C0)
#define CAM_REG_AE_GAIN2_0(module)  (isp_devs[module].regs + 0x09E0)
#define CAM_REG_AE_GAIN2_1(module)  (isp_devs[module].regs + 0x09E4)
#define CAM_REG_AE_LMT2_0(module)  (isp_devs[module].regs + 0x09E8)
#define CAM_REG_AE_LMT2_1(module)  (isp_devs[module].regs + 0x09EC)
#define CAM_REG_AE_RC_CNV_0(module)  (isp_devs[module].regs + 0x09F0)
#define CAM_REG_AE_RC_CNV_1(module)  (isp_devs[module].regs + 0x09F4)
#define CAM_REG_AE_RC_CNV_2(module)  (isp_devs[module].regs + 0x09F8)
#define CAM_REG_AE_RC_CNV_3(module)  (isp_devs[module].regs + 0x09FC)
#define CAM_REG_AE_RC_CNV_4(module)  (isp_devs[module].regs + 0x0A00)
#define CAM_REG_AE_YGAMMA_0(module)  (isp_devs[module].regs + 0x0A04)
#define CAM_REG_AE_YGAMMA_1(module)  (isp_devs[module].regs + 0x0A08)
#define CAM_REG_AE_HST0_RNG(module)  (isp_devs[module].regs + 0x09F4)
#define CAM_REG_AE_HST1_RNG(module)  (isp_devs[module].regs + 0x09F8)
#define CAM_REG_AE_HST2_RNG(module)  (isp_devs[module].regs + 0x09FC)
#define CAM_REG_AE_HST3_RNG(module)  (isp_devs[module].regs + 0x0A00)
#define CAM_REG_AE_OVER_EXPO_CFG(module)  (isp_devs[module].regs + 0x0A0C)
#define CAM_REG_AE_PIX_HST_CTL(module)    (isp_devs[module].regs + 0x0A10)
#define CAM_REG_AE_PIX_HST_SET(module)    (isp_devs[module].regs + 0x0A14)
#define CAM_REG_AE_PIX_HST0_YRNG(module)  (isp_devs[module].regs + 0x0A1C)
#define CAM_REG_AE_PIX_HST0_XRNG(module)  (isp_devs[module].regs + 0x0A20)
#define CAM_REG_AE_PIX_HST1_YRNG(module)  (isp_devs[module].regs + 0x0A24)
#define CAM_REG_AE_PIX_HST1_XRNG(module)  (isp_devs[module].regs + 0x0A28)
#define CAM_REG_AE_PIX_HST2_YRNG(module)  (isp_devs[module].regs + 0x0A2C)
#define CAM_REG_AE_PIX_HST2_XRNG(module)  (isp_devs[module].regs + 0x0A30)
#define CAM_REG_AE_PIX_HST3_YRNG(module)  (isp_devs[module].regs + 0x0A34)
#define CAM_REG_AE_PIX_HST3_XRNG(module)  (isp_devs[module].regs + 0x0A38)
#define CAM_REG_AE_STAT_EN(module)        (isp_devs[module].regs + 0x0A3C)
#define CAM_REG_AE_YCOEF(module)          (isp_devs[module].regs + 0x0A40)
#define CAM_REG_AE_CCU_HST_END_Y(module)  (isp_devs[module].regs + 0x0A44)
#define CAM_REG_AE_SPARE(module)          (isp_devs[module].regs + 0x0A48)
#define CAM_REG_QBN1_MODE(module)  (isp_devs[module].regs + 0x0AC0)
#define CAM_REG_CPG_SATU_1(module)  (isp_devs[module].regs + 0x0AD0)
#define CAM_REG_CPG_SATU_2(module)  (isp_devs[module].regs + 0x0AD4)
#define CAM_REG_CPG_GAIN_1(module)  (isp_devs[module].regs + 0x0AD8)
#define CAM_REG_CPG_GAIN_2(module)  (isp_devs[module].regs + 0x0ADC)
#define CAM_REG_CPG_OFST_1(module)  (isp_devs[module].regs + 0x0AE0)
#define CAM_REG_CPG_OFST_2(module)  (isp_devs[module].regs + 0x0AE4)
#define CAM_REG_CAC_TILE_SIZE(module)  (isp_devs[module].regs + 0x0AF0)
#define CAM_REG_CAC_TILE_OFFSET(module)  (isp_devs[module].regs + 0x0AF4)
#define CAM_REG_PMX_CTL(module)  (isp_devs[module].regs + 0x0B00)
#define CAM_REG_PMX_CROP(module)  (isp_devs[module].regs + 0x0B04)
#define CAM_REG_PMX_VSIZE(module)  (isp_devs[module].regs + 0x0B08)
#define CAM_REG_VBN_GAIN(module)  (isp_devs[module].regs + 0x0B40)
#define CAM_REG_VBN_OFST(module)  (isp_devs[module].regs + 0x0B44)
#define CAM_REG_VBN_TYPE(module)  (isp_devs[module].regs + 0x0B48)
#define CAM_REG_VBN_SPARE(module)  (isp_devs[module].regs + 0x0B4C)
#define CAM_REG_AMX_CTL(module)  (isp_devs[module].regs + 0x0B60)
#define CAM_REG_AMX_CROP(module)  (isp_devs[module].regs + 0x0B64)
#define CAM_REG_AMX_VSIZE(module)  (isp_devs[module].regs + 0x0B68)
#define CAM_REG_DBS_SIGMA(module)  (isp_devs[module].regs + 0x0B40)
#define CAM_REG_DBS_BSTBL_0(module)  (isp_devs[module].regs + 0x0B44)
#define CAM_REG_DBS_BSTBL_1(module)  (isp_devs[module].regs + 0x0B48)
#define CAM_REG_DBS_BSTBL_2(module)  (isp_devs[module].regs + 0x0B4C)
#define CAM_REG_DBS_BSTBL_3(module)  (isp_devs[module].regs + 0x0B50)
#define CAM_REG_DBS_CTL(module)  (isp_devs[module].regs + 0x0B54)
#define CAM_REG_DBS_CTL_2(module)  (isp_devs[module].regs + 0x0B58)
#define CAM_REG_DBS_SIGMA_2(module)  (isp_devs[module].regs + 0x0C1C)
#define CAM_REG_DBS_YGN(module)  (isp_devs[module].regs + 0x0C20)
#define CAM_REG_DBS_SL_Y12(module)  (isp_devs[module].regs + 0x0C24)
#define CAM_REG_DBS_SL_Y34(module)  (isp_devs[module].regs + 0x0C28)
#define CAM_REG_DBS_SL_G12(module)  (isp_devs[module].regs + 0x0C2C)
#define CAM_REG_DBS_SL_G34(module)  (isp_devs[module].regs + 0x0C30)
#define CAM_REG_BIN_CTL(module)  (isp_devs[module].regs + 0x0B80)
#define CAM_REG_BIN_FTH(module)  (isp_devs[module].regs + 0x0B84)
#define CAM_REG_BIN_SPARE(module)  (isp_devs[module].regs + 0x0B88)
#define CAM_REG_DBN_GAIN(module)  (isp_devs[module].regs + 0x0BA0)
#define CAM_REG_DBN_OFST(module)  (isp_devs[module].regs + 0x0BA4)
#define CAM_REG_DBN_SPARE(module)  (isp_devs[module].regs + 0x0BA8)
#define CAM_REG_PBN_TYPE(module)  (isp_devs[module].regs + 0x0BB0)
#define CAM_REG_PBN_LST(module)  (isp_devs[module].regs + 0x0BB4)
#define CAM_REG_PBN_VSIZE(module)  (isp_devs[module].regs + 0x0BB8)
#define CAM_REG_RCP3_CROP_CON1(module)  (isp_devs[module].regs + 0x0BC0)
#define CAM_REG_RCP3_CROP_CON2(module)  (isp_devs[module].regs + 0x0BC4)
#define CAM_REG_QBN4_MODE(module)  (isp_devs[module].regs + 0x0D00)
#define CAM_REG_PS_AWB_WIN_ORG(module)  (isp_devs[module].regs + 0x0D10)
#define CAM_REG_PS_AWB_WIN_SIZE(module)  (isp_devs[module].regs + 0x0D14)
#define CAM_REG_PS_AWB_WIN_PIT(module)  (isp_devs[module].regs + 0x0D18)
#define CAM_REG_PS_AWB_WIN_NUM(module)  (isp_devs[module].regs + 0x0D1C)
#define CAM_REG_PS_AWB_PIXEL_CNT0(module)  (isp_devs[module].regs + 0x0D20)
#define CAM_REG_PS_AWB_PIXEL_CNT1(module)  (isp_devs[module].regs + 0x0D24)
#define CAM_REG_PS_AWB_PIXEL_CNT2(module)  (isp_devs[module].regs + 0x0D28)
#define CAM_REG_PS_AWB_PIXEL_CNT3(module)  (isp_devs[module].regs + 0x0D2C)
#define CAM_REG_PS_AE_YCOEF0(module)  (isp_devs[module].regs + 0x0D30)
#define CAM_REG_PS_AE_YCOEF1(module)  (isp_devs[module].regs + 0x0D34)
#define CAM_REG_PSO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0D80)
#define CAM_REG_PSO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0D88)
#define CAM_REG_PSO_XSIZE(module)  (isp_devs[module].regs + 0x0D90)
#define CAM_REG_PSO_YSIZE(module)  (isp_devs[module].regs + 0x0D94)
#define CAM_REG_PSO_STRIDE(module)  (isp_devs[module].regs + 0x0D98)
#define CAM_REG_PSO_CON(module)  (isp_devs[module].regs + 0x0D9C)
#define CAM_REG_PSO_CON2(module)  (isp_devs[module].regs + 0x0DA0)
#define CAM_REG_PSO_CON3(module)  (isp_devs[module].regs + 0x0DA4)
#define CAM_REG_PSO_CON4(module)  (isp_devs[module].regs + 0x0DA8)
#define CAM_REG_PSO_ERR_STAT(module)  (isp_devs[module].regs + 0x0DAC)
#define CAM_REG_PSO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0DB0)
#define CAM_REG_PSO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0DB4)
#define CAM_REG_PSO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0DB8)
#define CAM_REG_PSO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0DBC)
#define CAM_REG_PSO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0DC0)
#define CAM_REG_PSO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0DC4)
#define CAM_REG_PSO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0DC8)
#define CAM_REG_PSO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0DCC)
#define CAM_REG_PSO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0DD0)
#define CAM_REG_PSO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0DD4)
#define CAM_REG_PSO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0DD8)
#define CAM_REG_PSO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0DDC)
#define CAM_REG_PSO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0DE0)
#define CAM_REG_PSO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0DE4)
#define CAM_REG_PSO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0DE8)
#define CAM_REG_DMA_FRAME_HEADER_EN(module)  (isp_devs[module].regs + 0x0E00)
#define CAM_REG_IMGO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E04)
#define CAM_REG_RRZO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E08)
#define CAM_REG_AAO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E0C)
#define CAM_REG_AFO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E10)
#define CAM_REG_LCSO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E14)
#define CAM_REG_UFEO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E18)
#define CAM_REG_PDO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E1C)
#define CAM_REG_PSO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E20)
#define CAM_REG_IMGO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0E30)
#define CAM_REG_IMGO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0E34)
#define CAM_REG_IMGO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0E38)
#define CAM_REG_IMGO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0E3C)
#define CAM_REG_IMGO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0E40)
#define CAM_REG_IMGO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0E44)
#define CAM_REG_IMGO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0E48)
#define CAM_REG_IMGO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0E4C)
#define CAM_REG_IMGO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0E50)
#define CAM_REG_IMGO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0E54)
#define CAM_REG_IMGO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0E58)
#define CAM_REG_IMGO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0E5C)
#define CAM_REG_IMGO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0E60)
#define CAM_REG_IMGO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0E64)
#define CAM_REG_IMGO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0E68)
#define CAM_REG_RRZO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0E70)
#define CAM_REG_RRZO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0E74)
#define CAM_REG_RRZO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0E78)
#define CAM_REG_RRZO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0E7C)
#define CAM_REG_RRZO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0E80)
#define CAM_REG_RRZO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0E84)
#define CAM_REG_RRZO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0E88)
#define CAM_REG_RRZO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0E8C)
#define CAM_REG_RRZO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0E90)
#define CAM_REG_RRZO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0E94)
#define CAM_REG_RRZO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0E98)
#define CAM_REG_RRZO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0E9C)
#define CAM_REG_RRZO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0EA0)
#define CAM_REG_RRZO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0EA4)
#define CAM_REG_RRZO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0EA8)
#define CAM_REG_AAO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0EB0)
#define CAM_REG_AAO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0EB4)
#define CAM_REG_AAO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0EB8)
#define CAM_REG_AAO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0EBC)
#define CAM_REG_AAO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0EC0)
#define CAM_REG_AAO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0EC4)
#define CAM_REG_AAO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0EC8)
#define CAM_REG_AAO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0ECC)
#define CAM_REG_AAO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0ED0)
#define CAM_REG_AAO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0ED4)
#define CAM_REG_AAO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0ED8)
#define CAM_REG_AAO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0EDC)
#define CAM_REG_AAO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0EE0)
#define CAM_REG_AAO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0EE4)
#define CAM_REG_AAO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0EE8)
#define CAM_REG_AFO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0EF0)
#define CAM_REG_AFO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0EF4)
#define CAM_REG_AFO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0EF8)
#define CAM_REG_AFO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0EFC)
#define CAM_REG_AFO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0F00)
#define CAM_REG_AFO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0F04)
#define CAM_REG_AFO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0F08)
#define CAM_REG_AFO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0F0C)
#define CAM_REG_AFO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0F10)
#define CAM_REG_AFO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0F14)
#define CAM_REG_AFO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0F18)
#define CAM_REG_AFO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0F1C)
#define CAM_REG_AFO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0F20)
#define CAM_REG_AFO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0F24)
#define CAM_REG_AFO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0F28)
#define CAM_REG_LCSO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0F30)
#define CAM_REG_LCSO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0F34)
#define CAM_REG_LCSO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0F38)
#define CAM_REG_LCSO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0F3C)
#define CAM_REG_LCSO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0F40)
#define CAM_REG_LCSO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0F44)
#define CAM_REG_LCSO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0F48)
#define CAM_REG_LCSO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0F4C)
#define CAM_REG_LCSO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0F50)
#define CAM_REG_LCSO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0F54)
#define CAM_REG_LCSO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0F58)
#define CAM_REG_LCSO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0F5C)
#define CAM_REG_LCSO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0F60)
#define CAM_REG_LCSO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0F64)
#define CAM_REG_LCSO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0F68)
#define CAM_REG_UFEO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0F70)
#define CAM_REG_UFEO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0F74)
#define CAM_REG_UFEO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0F78)
#define CAM_REG_UFEO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0F7C)
#define CAM_REG_UFEO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0F80)
#define CAM_REG_UFEO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0F84)
#define CAM_REG_UFEO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0F88)
#define CAM_REG_UFEO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0F8C)
#define CAM_REG_UFEO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0F90)
#define CAM_REG_UFEO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0F94)
#define CAM_REG_UFEO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0F98)
#define CAM_REG_UFEO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0F9C)
#define CAM_REG_UFEO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0FA0)
#define CAM_REG_UFEO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0FA4)
#define CAM_REG_UFEO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0FA8)
#define CAM_REG_PDO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0FB0)
#define CAM_REG_PDO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0FB4)
#define CAM_REG_PDO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0FB8)
#define CAM_REG_PDO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0FBC)
#define CAM_REG_PDO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0FC0)
#define CAM_REG_PDO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0FC4)
#define CAM_REG_PDO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0FC8)
#define CAM_REG_PDO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0FCC)
#define CAM_REG_PDO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0FD0)
#define CAM_REG_PDO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0FD4)
#define CAM_REG_PDO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0FD8)
#define CAM_REG_PDO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0FDC)
#define CAM_REG_PDO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0FE0)
#define CAM_REG_PDO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0FE4)
#define CAM_REG_PDO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0FE8)

/* CAM_UNI */
#define CAM_UNI_REG_TOP_CTL(module)  (isp_devs[module].regs + 0x0000)
#define CAM_UNI_REG_TOP_MISC(module)  (isp_devs[module].regs + 0x0004)
#define CAM_UNI_REG_TOP_SW_CTL(module)  (isp_devs[module].regs + 0x0008)
#define CAM_UNI_REG_TOP_RAWI_TRIG(module)  (isp_devs[module].regs + 0x000C)
#define CAM_UNI_REG_TOP_MOD_EN(module)  (isp_devs[module].regs + 0x0010)
#define CAM_UNI_REG_TOP_DMA_EN(module)  (isp_devs[module].regs + 0x0014)
#define CAM_UNI_REG_TOP_PATH_SEL(module)  (isp_devs[module].regs + 0x0018)
#define CAM_UNI_REG_TOP_FMT_SEL(module)  (isp_devs[module].regs + 0x001C)
#define CAM_UNI_REG_TOP_DMA_INT_EN(module)  (isp_devs[module].regs + 0x0020)
#define CAM_UNI_REG_TOP_DMA_INT_STATUS(module)  (isp_devs[module].regs + 0x0024)
#define CAM_UNI_REG_TOP_DMA_INT_STATUSX(module) \
		(isp_devs[module].regs + 0x0028)
#define CAM_UNI_REG_TOP_DBG_SET(module)  (isp_devs[module].regs + 0x002C)
#define CAM_UNI_REG_TOP_DBG_PORT(module)  (isp_devs[module].regs + 0x0030)
#define CAM_UNI_REG_TOP_MOD_DCM_DIS(module)  (isp_devs[module].regs + 0x0040)
#define CAM_UNI_REG_TOP_DMA_DCM_DIS(module)  (isp_devs[module].regs + 0x0044)
#define CAM_UNI_REG_TOP_MOD_DCM_STATUS(module)  (isp_devs[module].regs + 0x0050)
#define CAM_UNI_REG_TOP_DMA_DCM_STATUS(module)  (isp_devs[module].regs + 0x0054)
#define CAM_UNI_REG_TOP_MOD_REQ_STATUS(module)  (isp_devs[module].regs + 0x0060)
#define CAM_UNI_REG_TOP_DMA_REQ_STATUS(module)  (isp_devs[module].regs + 0x0064)
#define CAM_UNI_REG_TOP_MOD_RDY_STATUS(module)  (isp_devs[module].regs + 0x0070)
#define CAM_UNI_REG_TOP_DMA_RDY_STATUS(module)  (isp_devs[module].regs + 0x0074)
#define CAM_UNI_REG_FBC_FLKO_A_CTL1(module)  (isp_devs[module].regs + 0x0080)
#define CAM_UNI_REG_FBC_FLKO_A_CTL2(module)  (isp_devs[module].regs + 0x0084)
#define CAM_UNI_REG_FBC_EISO_A_CTL1(module)  (isp_devs[module].regs + 0x0088)
#define CAM_UNI_REG_FBC_EISO_A_CTL2(module)  (isp_devs[module].regs + 0x008C)
#define CAM_UNI_REG_FBC_RSSO_A_CTL1(module)  (isp_devs[module].regs + 0x0090)
#define CAM_UNI_REG_FBC_RSSO_A_CTL2(module)  (isp_devs[module].regs + 0x0094)
#define CAM_UNI_REG_DMA_SOFT_RSTSTAT(module)  (isp_devs[module].regs + 0x0200)
#define CAM_UNI_REG_VERTICAL_FLIP_EN(module)  (isp_devs[module].regs + 0x0204)
#define CAM_UNI_REG_DMA_SOFT_RESET(module)  (isp_devs[module].regs + 0x0208)
#define CAM_UNI_REG_LAST_ULTRA_EN(module)  (isp_devs[module].regs + 0x020C)
#define CAM_UNI_REG_SPECIAL_FUN_EN(module)  (isp_devs[module].regs + 0x0210)
#define CAM_UNI_REG_EISO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0220)
#define CAM_UNI_REG_EISO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0228)
#define CAM_UNI_REG_EISO_XSIZE(module)  (isp_devs[module].regs + 0x0230)
#define CAM_UNI_REG_EISO_YSIZE(module)  (isp_devs[module].regs + 0x0234)
#define CAM_UNI_REG_EISO_STRIDE(module)  (isp_devs[module].regs + 0x0238)
#define CAM_UNI_REG_EISO_CON(module)  (isp_devs[module].regs + 0x023C)
#define CAM_UNI_REG_EISO_CON2(module)  (isp_devs[module].regs + 0x0240)
#define CAM_UNI_REG_EISO_CON3(module)  (isp_devs[module].regs + 0x0244)
#define CAM_UNI_REG_FLKO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0250)
#define CAM_UNI_REG_FLKO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0258)
#define CAM_UNI_REG_FLKO_XSIZE(module)  (isp_devs[module].regs + 0x0260)
#define CAM_UNI_REG_FLKO_YSIZE(module)  (isp_devs[module].regs + 0x0264)
#define CAM_UNI_REG_FLKO_STRIDE(module)  (isp_devs[module].regs + 0x0268)
#define CAM_UNI_REG_FLKO_CON(module)  (isp_devs[module].regs + 0x026C)
#define CAM_UNI_REG_FLKO_CON2(module)  (isp_devs[module].regs + 0x0270)
#define CAM_UNI_REG_FLKO_CON3(module)  (isp_devs[module].regs + 0x0274)
#define CAM_UNI_REG_RSSO_A_BASE_ADDR(module)  (isp_devs[module].regs + 0x0280)
#define CAM_UNI_REG_RSSO_A_OFST_ADDR(module)  (isp_devs[module].regs + 0x0288)
#define CAM_UNI_REG_RSSO_A_XSIZE(module)  (isp_devs[module].regs + 0x0290)
#define CAM_UNI_REG_RSSO_A_YSIZE(module)  (isp_devs[module].regs + 0x0294)
#define CAM_UNI_REG_RSSO_A_STRIDE(module)  (isp_devs[module].regs + 0x0298)
#define CAM_UNI_REG_RSSO_A_CON(module)  (isp_devs[module].regs + 0x029C)
#define CAM_UNI_REG_RSSO_A_CON2(module)  (isp_devs[module].regs + 0x02A0)
#define CAM_UNI_REG_RSSO_A_CON3(module)  (isp_devs[module].regs + 0x02A4)
#define CAM_UNI_REG_RSSO_B_BASE_ADDR(module)  (isp_devs[module].regs + 0x02B0)
#define CAM_UNI_REG_RSSO_B_OFST_ADDR(module)  (isp_devs[module].regs + 0x02B8)
#define CAM_UNI_REG_RSSO_B_XSIZE(module)  (isp_devs[module].regs + 0x02C0)
#define CAM_UNI_REG_RSSO_B_YSIZE(module)  (isp_devs[module].regs + 0x02C4)
#define CAM_UNI_REG_RSSO_B_STRIDE(module)  (isp_devs[module].regs + 0x02C8)
#define CAM_UNI_REG_RSSO_B_CON(module)  (isp_devs[module].regs + 0x02CC)
#define CAM_UNI_REG_RSSO_B_CON2(module)  (isp_devs[module].regs + 0x02D0)
#define CAM_UNI_REG_RSSO_B_CON3(module)  (isp_devs[module].regs + 0x02D4)
#define CAM_UNI_REG_RAWI_BASE_ADDR(module)  (isp_devs[module].regs + 0x0340)
#define CAM_UNI_REG_RAWI_OFST_ADDR(module)  (isp_devs[module].regs + 0x0348)
#define CAM_UNI_REG_RAWI_XSIZE(module)  (isp_devs[module].regs + 0x0350)
#define CAM_UNI_REG_RAWI_YSIZE(module)  (isp_devs[module].regs + 0x0354)
#define CAM_UNI_REG_RAWI_STRIDE(module)  (isp_devs[module].regs + 0x0358)
#define CAM_UNI_REG_RAWI_CON(module)  (isp_devs[module].regs + 0x035C)
#define CAM_UNI_REG_RAWI_CON2(module)  (isp_devs[module].regs + 0x0360)
#define CAM_UNI_REG_RAWI_CON3(module)  (isp_devs[module].regs + 0x0364)
#define CAM_UNI_REG_DMA_ERR_CTRL(module)  (isp_devs[module].regs + 0x0370)
#define CAM_UNI_REG_EISO_ERR_STAT(module)  (isp_devs[module].regs + 0x0374)
#define CAM_UNI_REG_FLKO_ERR_STAT(module)  (isp_devs[module].regs + 0x0378)
#define CAM_UNI_REG_RSSO_A_ERR_STAT(module)  (isp_devs[module].regs + 0x037C)
#define CAM_UNI_REG_RSSO_B_ERR_STAT(module)  (isp_devs[module].regs + 0x0380)
#define CAM_UNI_REG_RAWI_ERR_STAT(module)  (isp_devs[module].regs + 0x0384)
#define CAM_UNI_REG_DMA_DEBUG_ADDR(module)  (isp_devs[module].regs + 0x0388)
#define CAM_UNI_REG_DMA_RSV1(module)  (isp_devs[module].regs + 0x038C)
#define CAM_UNI_REG_DMA_RSV2(module)  (isp_devs[module].regs + 0x0390)
#define CAM_UNI_REG_DMA_RSV3(module)  (isp_devs[module].regs + 0x0394)
#define CAM_UNI_REG_DMA_RSV4(module)  (isp_devs[module].regs + 0x0398)
#define CAM_UNI_REG_DMA_RSV5(module)  (isp_devs[module].regs + 0x039C)
#define CAM_UNI_REG_DMA_RSV6(module)  (isp_devs[module].regs + 0x03A0)
#define CAM_UNI_REG_DMA_DEBUG_SEL(module)  (isp_devs[module].regs + 0x03A4)
#define CAM_UNI_REG_DMA_BW_SELF_TEST(module)  (isp_devs[module].regs + 0x03A8)
#define CAM_UNI_REG_DMA_FRAME_HEADER_EN(module)  \
		(isp_devs[module].regs + 0x03C0)
#define CAM_UNI_REG_EISO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x03C4)
#define CAM_UNI_REG_FLKO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x03C8)
#define CAM_UNI_REG_RSSO_A_FH_BASE_ADDR(module) \
		(isp_devs[module].regs + 0x03CC)
#define CAM_UNI_REG_RSSO_B_FH_BASE_ADDR(module) \
		(isp_devs[module].regs + 0x03D0)
#define CAM_UNI_REG_EISO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x03E0)
#define CAM_UNI_REG_EISO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x03E4)
#define CAM_UNI_REG_EISO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x03E8)
#define CAM_UNI_REG_EISO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x03EC)
#define CAM_UNI_REG_EISO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x03F0)
#define CAM_UNI_REG_EISO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x03F4)
#define CAM_UNI_REG_EISO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x03F8)
#define CAM_UNI_REG_EISO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x03FC)
#define CAM_UNI_REG_EISO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0400)
#define CAM_UNI_REG_EISO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0404)
#define CAM_UNI_REG_EISO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0408)
#define CAM_UNI_REG_EISO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x040C)
#define CAM_UNI_REG_EISO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0410)
#define CAM_UNI_REG_EISO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0414)
#define CAM_UNI_REG_EISO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0418)
#define CAM_UNI_REG_FLKO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0420)
#define CAM_UNI_REG_FLKO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0424)
#define CAM_UNI_REG_FLKO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0428)
#define CAM_UNI_REG_FLKO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x042C)
#define CAM_UNI_REG_FLKO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0430)
#define CAM_UNI_REG_FLKO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0434)
#define CAM_UNI_REG_FLKO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0438)
#define CAM_UNI_REG_FLKO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x043C)
#define CAM_UNI_REG_FLKO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0440)
#define CAM_UNI_REG_FLKO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0444)
#define CAM_UNI_REG_FLKO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0448)
#define CAM_UNI_REG_FLKO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x044C)
#define CAM_UNI_REG_FLKO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0450)
#define CAM_UNI_REG_FLKO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0454)
#define CAM_UNI_REG_FLKO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0458)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0460)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0464)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0468)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_5(module)  (isp_devs[module].regs + 0x046C)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0470)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0474)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0478)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_9(module)  (isp_devs[module].regs + 0x047C)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0480)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0484)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0488)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_13(module)  (isp_devs[module].regs + 0x048C)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0490)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0494)
#define CAM_UNI_REG_RSSO_A_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0498)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_2(module)  (isp_devs[module].regs + 0x04A0)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_3(module)  (isp_devs[module].regs + 0x04A4)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_4(module)  (isp_devs[module].regs + 0x04A8)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_5(module)  (isp_devs[module].regs + 0x04AC)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_6(module)  (isp_devs[module].regs + 0x04B0)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_7(module)  (isp_devs[module].regs + 0x04B4)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_8(module)  (isp_devs[module].regs + 0x04B8)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_9(module)  (isp_devs[module].regs + 0x04BC)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_10(module)  (isp_devs[module].regs + 0x04C0)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_11(module)  (isp_devs[module].regs + 0x04C4)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_12(module)  (isp_devs[module].regs + 0x04C8)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_13(module)  (isp_devs[module].regs + 0x04CC)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_14(module)  (isp_devs[module].regs + 0x04D0)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_15(module)  (isp_devs[module].regs + 0x04D4)
#define CAM_UNI_REG_RSSO_B_FH_SPARE_16(module)  (isp_devs[module].regs + 0x04D8)
#define CAM_UNI_REG_UNP2_A_OFST(module)  (isp_devs[module].regs + 0x0500)
#define CAM_UNI_REG_QBN3_A_MODE(module)  (isp_devs[module].regs + 0x0510)
#define CAM_UNI_REG_SGG3_A_PGN(module)  (isp_devs[module].regs + 0x0520)
#define CAM_UNI_REG_SGG3_A_GMRC_1(module)  (isp_devs[module].regs + 0x0524)
#define CAM_UNI_REG_SGG3_A_GMRC_2(module)  (isp_devs[module].regs + 0x0528)
#define CAM_UNI_REG_FLK_A_CON(module)  (isp_devs[module].regs + 0x0530)
#define CAM_UNI_REG_FLK_A_OFST(module)  (isp_devs[module].regs + 0x0534)
#define CAM_UNI_REG_FLK_A_SIZE(module)  (isp_devs[module].regs + 0x0538)
#define CAM_UNI_REG_FLK_A_NUM(module)  (isp_devs[module].regs + 0x053C)
#define CAM_UNI_REG_EIS_A_PREP_ME_CTRL1(module) \
		(isp_devs[module].regs + 0x0550)
#define CAM_UNI_REG_EIS_A_PREP_ME_CTRL2(module) \
		(isp_devs[module].regs + 0x0554)
#define CAM_UNI_REG_EIS_A_LMV_TH(module)  (isp_devs[module].regs + 0x0558)
#define CAM_UNI_REG_EIS_A_FL_OFFSET(module)  (isp_devs[module].regs + 0x055C)
#define CAM_UNI_REG_EIS_A_MB_OFFSET(module)  (isp_devs[module].regs + 0x0560)
#define CAM_UNI_REG_EIS_A_MB_INTERVAL(module)  (isp_devs[module].regs + 0x0564)
#define CAM_UNI_REG_EIS_A_GMV(module)  (isp_devs[module].regs + 0x0568)
#define CAM_UNI_REG_EIS_A_ERR_CTRL(module)  (isp_devs[module].regs + 0x056C)
#define CAM_UNI_REG_EIS_A_IMAGE_CTRL(module)  (isp_devs[module].regs + 0x0570)
#define CAM_UNI_REG_SGG2_A_PGN(module)  (isp_devs[module].regs + 0x0590)
#define CAM_UNI_REG_SGG2_A_GMRC_1(module)  (isp_devs[module].regs + 0x0594)
#define CAM_UNI_REG_SGG2_A_GMRC_2(module)  (isp_devs[module].regs + 0x0598)
#define CAM_UNI_REG_HDS_A_MODE(module)  (isp_devs[module].regs + 0x05A0)
#define CAM_UNI_REG_RSS_A_CONTROL(module)  (isp_devs[module].regs + 0x05C0)
#define CAM_UNI_REG_RSS_A_IN_IMG(module)  (isp_devs[module].regs + 0x05C4)
#define CAM_UNI_REG_RSS_A_OUT_IMG(module)  (isp_devs[module].regs + 0x05C8)
#define CAM_UNI_REG_RSS_A_HORI_STEP(module)  (isp_devs[module].regs + 0x05CC)
#define CAM_UNI_REG_RSS_A_VERT_STEP(module)  (isp_devs[module].regs + 0x05D0)
#define CAM_UNI_REG_RSS_A_HORI_INT_OFST(module) \
		(isp_devs[module].regs + 0x05D4)
#define CAM_UNI_REG_RSS_A_HORI_SUB_OFST(module) \
		(isp_devs[module].regs + 0x05D8)
#define CAM_UNI_REG_RSS_A_VERT_INT_OFST(module) \
		(isp_devs[module].regs + 0x05DC)
#define CAM_UNI_REG_RSS_A_VERT_SUB_OFST(module) \
		(isp_devs[module].regs + 0x05E0)
#define CAM_UNI_REG_RSS_A_SPARE_1(module)  (isp_devs[module].regs + 0x05F4)

/* CAMSV */
#define CAMSV_REG_TOP_DEBUG(module)  (isp_devs[module].regs + 0x0000)
#define CAMSV_REG_MODULE_EN(module)  (isp_devs[module].regs + 0x0010)
#define CAMSV_REG_FMT_SEL(module)  (isp_devs[module].regs + 0x0014)
#define CAMSV_REG_INT_EN(module)  (isp_devs[module].regs + 0x0018)
#define CAMSV_REG_INT_STATUS(module)  (isp_devs[module].regs + 0x001C)
#define CAMSV_REG_SW_CTL(module)  (isp_devs[module].regs + 0x0020)
#define CAMSV_REG_SPARE0(module)  (isp_devs[module].regs + 0x0024)
#define CAMSV_REG_SPARE1(module)  (isp_devs[module].regs + 0x0028)
#define CAMSV_REG_IMGO_FBC(module)  (isp_devs[module].regs + 0x002C)
#define CAMSV_REG_CLK_EN(module)  (isp_devs[module].regs + 0x0030)
#define CAMSV_REG_DBG_SET(module)  (isp_devs[module].regs + 0x0034)
#define CAMSV_REG_DBG_PORT(module)  (isp_devs[module].regs + 0x0038)
#define CAMSV_REG_DATE_CODE(module)  (isp_devs[module].regs + 0x003C)
#define CAMSV_REG_PROJ_CODE(module)  (isp_devs[module].regs + 0x0040)
#define CAMSV_REG_DCM_DIS(module)  (isp_devs[module].regs + 0x0044)
#define CAMSV_REG_DCM_STATUS(module)  (isp_devs[module].regs + 0x0048)
#define CAMSV_REG_PAK(module)  (isp_devs[module].regs + 0x004C)
#define CAMSV_REG_FBC_IMGO_CTL1(module)  (isp_devs[module].regs + 0x0110)
#define CAMSV_REG_FBC_IMGO_CTL2(module)  (isp_devs[module].regs + 0x0114)
#define CAMSV_REG_FBC_IMGO_ENQ_ADDR(module)  (isp_devs[module].regs + 0x0118)
#define CAMSV_REG_FBC_IMGO_CUR_ADDR(module)  (isp_devs[module].regs + 0x011C)
#define CAMSV_REG_DMA_SOFT_RSTSTAT(module)  (isp_devs[module].regs + 0x0200)
#define CAMSV_REG_CQ0I_BASE_ADDR(module)  (isp_devs[module].regs + 0x0204)
#define CAMSV_REG_CQ0I_XSIZE(module)  (isp_devs[module].regs + 0x0208)
#define CAMSV_REG_VERTICAL_FLIP_EN(module)  (isp_devs[module].regs + 0x020C)
#define CAMSV_REG_DMA_SOFT_RESET(module)  (isp_devs[module].regs + 0x0210)
#define CAMSV_REG_LAST_ULTRA_EN(module)  (isp_devs[module].regs + 0x0214)
#define CAMSV_REG_SPECIAL_FUN_EN(module)  (isp_devs[module].regs + 0x0218)
#define CAMSV_REG_IMGO_BASE_ADDR(module)  (isp_devs[module].regs + 0x0220)
#define CAMSV_REG_IMGO_OFST_ADDR(module)  (isp_devs[module].regs + 0x0228)
#define CAMSV_REG_IMGO_XSIZE(module)  (isp_devs[module].regs + 0x0230)
#define CAMSV_REG_IMGO_YSIZE(module)  (isp_devs[module].regs + 0x0234)
#define CAMSV_REG_IMGO_STRIDE(module)  (isp_devs[module].regs + 0x0238)
#define CAMSV_REG_IMGO_CON(module)  (isp_devs[module].regs + 0x023C)
#define CAMSV_REG_IMGO_CON2(module)  (isp_devs[module].regs + 0x0240)
#define CAMSV_REG_IMGO_CON3(module)  (isp_devs[module].regs + 0x0244)
#define CAMSV_REG_IMGO_CROP(module)  (isp_devs[module].regs + 0x0248)
#define CAMSV_REG_DMA_ERR_CTRL(module)  (isp_devs[module].regs + 0x0440)
#define CAMSV_REG_IMGO_ERR_STAT(module)  (isp_devs[module].regs + 0x0444)
#define CAMSV_REG_DMA_DEBUG_ADDR(module)  (isp_devs[module].regs + 0x046C)
#define CAMSV_REG_DMA_RSV1(module)  (isp_devs[module].regs + 0x0470)
#define CAMSV_REG_DMA_RSV2(module)  (isp_devs[module].regs + 0x0474)
#define CAMSV_REG_DMA_RSV3(module)  (isp_devs[module].regs + 0x0478)
#define CAMSV_REG_DMA_RSV4(module)  (isp_devs[module].regs + 0x047C)
#define CAMSV_REG_DMA_RSV5(module)  (isp_devs[module].regs + 0x0480)
#define CAMSV_REG_DMA_RSV6(module)  (isp_devs[module].regs + 0x0484)
#define CAMSV_REG_DMA_DEBUG_SEL(module)  (isp_devs[module].regs + 0x0488)
#define CAMSV_REG_DMA_BW_SELF_TEST(module)  (isp_devs[module].regs + 0x048C)
#define CAMSV_REG_TG_SEN_MODE(module)  (isp_devs[module].regs + 0x0500)
#define CAMSV_REG_TG_VF_CON(module)  (isp_devs[module].regs + 0x0504)
#define CAMSV_REG_TG_SEN_GRAB_PXL(module)  (isp_devs[module].regs + 0x0508)
#define CAMSV_REG_TG_SEN_GRAB_LIN(module)  (isp_devs[module].regs + 0x050C)
#define CAMSV_REG_TG_PATH_CFG(module)  (isp_devs[module].regs + 0x0510)
#define CAMSV_REG_TG_MEMIN_CTL(module)  (isp_devs[module].regs + 0x0514)
#define CAMSV_REG_TG_INT1(module)  (isp_devs[module].regs + 0x0518)
#define CAMSV_REG_TG_INT2(module)  (isp_devs[module].regs + 0x051C)
#define CAMSV_REG_TG_SOF_CNT(module)  (isp_devs[module].regs + 0x0520)
#define CAMSV_REG_TG_SOT_CNT(module)  (isp_devs[module].regs + 0x0524)
#define CAMSV_REG_TG_EOT_CNT(module)  (isp_devs[module].regs + 0x0528)
#define CAMSV_REG_TG_ERR_CTL(module)  (isp_devs[module].regs + 0x052C)
#define CAMSV_REG_TG_DAT_NO(module)  (isp_devs[module].regs + 0x0530)
#define CAMSV_REG_TG_FRM_CNT_ST(module)  (isp_devs[module].regs + 0x0534)
#define CAMSV_REG_TG_FRMSIZE_ST(module)  (isp_devs[module].regs + 0x0538)
#define CAMSV_REG_TG_INTER_ST(module)  (isp_devs[module].regs + 0x053C)
#define CAMSV_REG_TG_FLASHA_CTL(module)  (isp_devs[module].regs + 0x0540)
#define CAMSV_REG_TG_FLASHA_LINE_CNT(module)  (isp_devs[module].regs + 0x0544)
#define CAMSV_REG_TG_FLASHA_POS(module)  (isp_devs[module].regs + 0x0548)
#define CAMSV_REG_TG_FLASHB_CTL(module)  (isp_devs[module].regs + 0x054C)
#define CAMSV_REG_TG_FLASHB_LINE_CNT(module)  (isp_devs[module].regs + 0x0550)
#define CAMSV_REG_TG_FLASHB_POS(module)  (isp_devs[module].regs + 0x0554)
#define CAMSV_REG_TG_FLASHB_POS1(module)  (isp_devs[module].regs + 0x0558)
#define CAMSV_REG_TG_I2C_CQ_TRIG(module)  (isp_devs[module].regs + 0x0560)
#define CAMSV_REG_TG_CQ_TIMING(module)  (isp_devs[module].regs + 0x0564)
#define CAMSV_REG_TG_CQ_NUM(module)  (isp_devs[module].regs + 0x0568)
#define CAMSV_REG_TG_TIME_STAMP(module)  (isp_devs[module].regs + 0x0570)
#define CAMSV_REG_TG_SUB_PERIOD(module)  (isp_devs[module].regs + 0x0574)
#define CAMSV_REG_TG_DAT_NO_R(module)  (isp_devs[module].regs + 0x0578)
#define CAMSV_REG_TG_FRMSIZE_ST_R(module)  (isp_devs[module].regs + 0x057C)
#define CAMSV_REG_DMA_FRAME_HEADER_EN(module)  (isp_devs[module].regs + 0x0E00)
#define CAMSV_REG_IMGO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E04)
#define CAMSV_REG_RRZO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E08)
#define CAMSV_REG_AAO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E0C)
#define CAMSV_REG_AFO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E10)
#define CAMSV_REG_LCSO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E14)
#define CAMSV_REG_UFEO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E18)
#define CAMSV_REG_PDO_FH_BASE_ADDR(module)  (isp_devs[module].regs + 0x0E1C)
#define CAMSV_REG_IMGO_FH_SPARE_2(module)  (isp_devs[module].regs + 0x0E30)
#define CAMSV_REG_IMGO_FH_SPARE_3(module)  (isp_devs[module].regs + 0x0E34)
#define CAMSV_REG_IMGO_FH_SPARE_4(module)  (isp_devs[module].regs + 0x0E38)
#define CAMSV_REG_IMGO_FH_SPARE_5(module)  (isp_devs[module].regs + 0x0E3C)
#define CAMSV_REG_IMGO_FH_SPARE_6(module)  (isp_devs[module].regs + 0x0E40)
#define CAMSV_REG_IMGO_FH_SPARE_7(module)  (isp_devs[module].regs + 0x0E44)
#define CAMSV_REG_IMGO_FH_SPARE_8(module)  (isp_devs[module].regs + 0x0E48)
#define CAMSV_REG_IMGO_FH_SPARE_9(module)  (isp_devs[module].regs + 0x0E4C)
#define CAMSV_REG_IMGO_FH_SPARE_10(module)  (isp_devs[module].regs + 0x0E50)
#define CAMSV_REG_IMGO_FH_SPARE_11(module)  (isp_devs[module].regs + 0x0E54)
#define CAMSV_REG_IMGO_FH_SPARE_12(module)  (isp_devs[module].regs + 0x0E58)
#define CAMSV_REG_IMGO_FH_SPARE_13(module)  (isp_devs[module].regs + 0x0E5C)
#define CAMSV_REG_IMGO_FH_SPARE_14(module)  (isp_devs[module].regs + 0x0E60)
#define CAMSV_REG_IMGO_FH_SPARE_15(module)  (isp_devs[module].regs + 0x0E64)
#define CAMSV_REG_IMGO_FH_SPARE_16(module)  (isp_devs[module].regs + 0x0E68)



#define ISP_REG_ADDR_EN1                (ISP_ADDR + 0x4)
#define ISP_REG_ADDR_INT_P1_ST          (ISP_ADDR + 0x4C)
#define CAM_REG_ADDR_DMA_ST             (ISP_ADDR + 0x4C)
#define ISP_REG_ADDR_INT_P1_ST2         (ISP_ADDR + 0x54)
#define ISP_REG_ADDR_INT_P1_ST_D        (ISP_ADDR + 0x5C)
#define ISP_REG_ADDR_INT_P1_ST2_D       (ISP_ADDR + 0x64)
#define ISP_REG_ADDR_INT_P2_ST          (ISP_ADDR + 0x6C)
#define ISP_REG_ADDR_INT_STATUSX        (ISP_ADDR + 0x70)
#define ISP_REG_ADDR_INT_STATUS2X       (ISP_ADDR + 0x74)
#define ISP_REG_ADDR_INT_STATUS3X       (ISP_ADDR + 0x78)
#define ISP_REG_ADDR_CAM_SW_CTL         (ISP_ADDR + 0x8C)
#define CAM_REG_ADDR_IMGO_FBC_1           (ISP_ADDR + 0xF0)
#define CAM_REG_ADDR_RRZO_FBC_1           (ISP_ADDR + 0xF4)
#define CAM_REG_ADDR_IMGO_FBC_2           (ISP_ADDR + 0xF0)
#define CAM_REG_ADDR_RRZO_FBC_2           (ISP_ADDR + 0xF4)


#define ISP_REG_ADDR_IMGO_D_FBC         (ISP_ADDR + 0xF8)
#define ISP_REG_ADDR_RRZO_D_FBC         (ISP_ADDR + 0xFC)
#define ISP_REG_ADDR_TG_VF_CON          (ISP_ADDR + 0x414)
#define ISP_REG_ADDR_TG_INTER_ST        (ISP_ADDR + 0x44C)
#define ISP_REG_ADDR_TG2_VF_CON         (ISP_ADDR + 0x2414)
#define ISP_REG_ADDR_TG2_INTER_ST       (ISP_ADDR + 0x244C)
#define ISP_REG_ADDR_IMGO_BASE_ADDR     (ISP_ADDR + 0x3300)
#define ISP_REG_ADDR_RRZO_BASE_ADDR     (ISP_ADDR + 0x3320)
#define ISP_REG_ADDR_IMGO_D_BASE_ADDR   (ISP_ADDR + 0x34D4)
#define ISP_REG_ADDR_RRZO_D_BASE_ADDR   (ISP_ADDR + 0x34F4)
#define ISP_REG_ADDR_SENINF1_INT        (ISP_ADDR + 0x4128)
#define ISP_REG_ADDR_SENINF2_INT        (ISP_ADDR + 0x4528)
#define ISP_REG_ADDR_SENINF3_INT        (ISP_ADDR + 0x4928)
#define ISP_REG_ADDR_SENINF4_INT        (ISP_ADDR + 0x4D28)
#define ISP_REG_ADDR_CAMSV_FMT_SEL      (ISP_ADDR + 0x5004)
#define ISP_REG_ADDR_CAMSV_INT          (ISP_ADDR + 0x500C)
#define ISP_REG_ADDR_CAMSV_SW_CTL       (ISP_ADDR + 0x5010)
#define ISP_REG_ADDR_CAMSV_TG_INTER_ST  (ISP_ADDR + 0x544C)
#define ISP_REG_ADDR_CAMSV2_FMT_SEL     (ISP_ADDR + 0x5804)
#define ISP_REG_ADDR_CAMSV2_INT         (ISP_ADDR + 0x580C)
#define ISP_REG_ADDR_CAMSV2_SW_CTL      (ISP_ADDR + 0x5810)
#define ISP_REG_ADDR_CAMSV_TG2_INTER_ST (ISP_ADDR + 0x5C4C)
#define ISP_REG_ADDR_CAMSV_IMGO_FBC     (ISP_ADDR + 0x501C)
#define ISP_REG_ADDR_CAMSV2_IMGO_FBC    (ISP_ADDR + 0x581C)
#define ISP_REG_ADDR_IMGO_SV_BASE_ADDR  (ISP_ADDR + 0x5208)
#define ISP_REG_ADDR_IMGO_SV_XSIZE      (ISP_ADDR + 0x5210)
#define ISP_REG_ADDR_IMGO_SV_YSIZE      (ISP_ADDR + 0x5214)
#define ISP_REG_ADDR_IMGO_SV_STRIDE     (ISP_ADDR + 0x5218)
#define ISP_REG_ADDR_IMGO_SV_D_BASE_ADDR    (ISP_ADDR + 0x5228)
#define ISP_REG_ADDR_IMGO_SV_D_XSIZE    (ISP_ADDR + 0x5230)
#define ISP_REG_ADDR_IMGO_SV_D_YSIZE    (ISP_ADDR + 0x5234)
#define ISP_REG_ADDR_IMGO_SV_D_STRIDE   (ISP_ADDR + 0x5238)
#define TG_REG_ADDR_GRAB_W              (ISP_ADDR + 0x418)
#define TG2_REG_ADDR_GRAB_W             (ISP_ADDR + 0x2418)
#define TG_REG_ADDR_GRAB_H              (ISP_ADDR + 0x41C)
#define TG2_REG_ADDR_GRAB_H             (ISP_ADDR + 0x241C)

#define ISP_REG_ADDR_CAMSV_TG_VF_CON    (ISP_ADDR + 0x5414)
#define ISP_REG_ADDR_CAMSV_TG2_VF_CON   (ISP_ADDR + 0x5C14)
#define CAMSV_REG_ADDR_IMGO_FBC_1           (ISP_ADDR + 0xF0)
#define CAMSV_REG_ADDR_IMGO_FBC_2           (ISP_ADDR + 0xF4)
#define CAMSV_REG_ADDR_TG_INTER_ST      (0)
/* spare register */
/* #define ISP_REG_ADDR_TG_MAGIC_0         (ISP_ADDR + 0x94) */
/* #define ISP_REG_ADDR_TG_MAGIC_1         (ISP_ADDR + 0x9C) */
/* New define by 20131114 */
#define ISP_REG_ADDR_TG_MAGIC_0         (ISP_IMGSYS_BASE + 0x75DC) /* 0088 */

#define ISP_REG_ADDR_TG2_MAGIC_0        (ISP_IMGSYS_BASE + 0x75E4) /* 0090 */

/* for rrz input crop size */
#define ISP_REG_ADDR_TG_RRZ_CROP_IN     (ISP_IMGSYS_BASE + 0x75E0)
#define ISP_REG_ADDR_TG_RRZ_CROP_IN_D   (ISP_IMGSYS_BASE + 0x75E8)

/* for rrz destination width
 * (in twin mode, ISP_INNER_REG_ADDR_RRZO_XSIZE < RRZO width)
 */
#define ISP_REG_ADDR_RRZ_W         (ISP_ADDR_CAMINF + 0x4094)/* /// */
#define ISP_REG_ADDR_RRZ_W_D       (ISP_ADDR_CAMINF + 0x409C)/* /// */
/*
* CAM_REG_CTL_SPARE1              CAM_CTL_SPARE1;                 //4094
* CAM_REG_CTL_SPARE2              CAM_CTL_SPARE2;                 //409C
* CAM_REG_CTL_SPARE3              CAM_CTL_SPARE3;                 //4100
* CAM_REG_AE_SPARE                 CAM_AE_SPARE;                   //4694
* CAM_REG_DM_O_SPARE             CAM_DM_O_SPARE;                 //48F0
* CAM_REG_MIX1_SPARE              CAM_MIX1_SPARE;                 //4C98
* CAM_REG_MIX2_SPARE              CAM_MIX2_SPARE;                 //4CA8
* CAM_REG_MIX3_SPARE              CAM_MIX3_SPARE;                 //4CB8
* CAM_REG_NR3D_SPARE0            CAM_NR3D_SPARE0;                //4D04
* CAM_REG_AWB_D_SPARE           CAM_AWB_D_SPARE;                //663C
* CAM_REG_AE_D_SPARE              CAM_AE_D_SPARE;                 //6694
* CAMSV_REG_CAMSV_SPARE0      CAMSV_CAMSV_SPARE0;             //9014
* CAMSV_REG_CAMSV_SPARE1      CAMSV_CAMSV_SPARE1;             //9018
* CAMSV_REG_CAMSV2_SPARE0    CAMSV_CAMSV2_SPARE0;            //9814
* CAMSV_REG_CAMSV2_SPARE1    CAMSV_CAMSV2_SPARE1;            //9818
*/

/* inner register */
/* 1500_d000 ==> 1500_4000 */
/* 1500_e000 ==> 1500_6000 */
/* 1500_f000 ==> 1500_7000 */
#define ISP_INNER_REG_ADDR_FMT_SEL_P1       (ISP_ADDR + 0x0028)
#define ISP_INNER_REG_ADDR_FMT_SEL_P1_D     (ISP_ADDR + 0x002C)
#define ISP_INNER_REG_ADDR_CAM_CTRL_SEL_P1       (ISP_ADDR + 0x0034)
#define ISP_INNER_REG_ADDR_CAM_CTRL_SEL_P1_D     (ISP_ADDR + 0x0038)
/* #define ISP_INNER_REG_ADDR_FMT_SEL_P1       (ISP_ADDR_CAMINF + 0xD028) */
/* #define ISP_INNER_REG_ADDR_FMT_SEL_P1_D     (ISP_ADDR_CAMINF + 0xD02C) */
#define ISP_INNER_REG_ADDR_IMGO_XSIZE       (ISP_ADDR_CAMINF + 0xF308)
#define ISP_INNER_REG_ADDR_IMGO_YSIZE       (ISP_ADDR_CAMINF + 0xF30C)
#define ISP_INNER_REG_ADDR_IMGO_STRIDE      (ISP_ADDR_CAMINF + 0xF310)
#define ISP_INNER_REG_ADDR_IMGO_CROP        (ISP_ADDR_CAMINF + 0xF31C)
#define ISP_INNER_REG_ADDR_RRZO_XSIZE       (ISP_ADDR_CAMINF + 0xF328)
#define ISP_INNER_REG_ADDR_RRZO_YSIZE       (ISP_ADDR_CAMINF + 0xF32C)
#define ISP_INNER_REG_ADDR_RRZO_STRIDE      (ISP_ADDR_CAMINF + 0xF330)
#define ISP_INNER_REG_ADDR_RRZO_CROP        (ISP_ADDR_CAMINF + 0xF33C)
#define ISP_INNER_REG_ADDR_IMGO_D_XSIZE     (ISP_ADDR_CAMINF + 0xF4DC)
#define ISP_INNER_REG_ADDR_IMGO_D_YSIZE     (ISP_ADDR_CAMINF + 0xF4E0)
#define ISP_INNER_REG_ADDR_IMGO_D_STRIDE    (ISP_ADDR_CAMINF + 0xF4E4)
#define ISP_INNER_REG_ADDR_IMGO_D_CROP      (ISP_ADDR_CAMINF + 0xF4F0)
#define ISP_INNER_REG_ADDR_RRZO_D_XSIZE     (ISP_ADDR_CAMINF + 0xF4FC)
#define ISP_INNER_REG_ADDR_RRZO_D_YSIZE     (ISP_ADDR_CAMINF + 0xF500)
#define ISP_INNER_REG_ADDR_RRZO_D_STRIDE    (ISP_ADDR_CAMINF + 0xF504)
#define ISP_INNER_REG_ADDR_RRZO_D_CROP      (ISP_ADDR_CAMINF + 0xF510)

#define ISP_INNER_REG_ADDR_RRZ_HORI_INT_OFST (ISP_ADDR_CAMINF + 0xD7B4)
#define ISP_INNER_REG_ADDR_RRZ_VERT_INT_OFST (ISP_ADDR_CAMINF + 0xD7BC)
#define ISP_INNER_REG_ADDR_RRZ_IN_IMG        (ISP_ADDR_CAMINF + 0xD7A4)
#define ISP_INNER_REG_ADDR_RRZ_OUT_IMG       (ISP_ADDR_CAMINF + 0xD7A8)

#define ISP_INNER_REG_ADDR_RRZ_D_HORI_INT_OFST (ISP_ADDR_CAMINF + 0xE7B4)
#define ISP_INNER_REG_ADDR_RRZ_D_VERT_INT_OFST (ISP_ADDR_CAMINF + 0xE7BC)
#define ISP_INNER_REG_ADDR_RRZ_D_IN_IMG        (ISP_ADDR_CAMINF + 0xE7A4)
#define ISP_INNER_REG_ADDR_RRZ_D_OUT_IMG       (ISP_ADDR_CAMINF + 0xE7A8)


#define ISP_TPIPE_ADDR                  (0x15004000)

/* CAM_CTL_SW_CTL */
#define ISP_REG_SW_CTL_SW_RST_P1_MASK   (0x00000007)
#define ISP_REG_SW_CTL_SW_RST_TRIG      (0x00000001)
#define ISP_REG_SW_CTL_SW_RST_STATUS    (0x00000002)
#define ISP_REG_SW_CTL_HW_RST           (0x00000004)
#define ISP_REG_SW_CTL_SW_RST_P2_MASK   (0x00000070)
#define ISP_REG_SW_CTL_SW_RST_P2_TRIG   (0x00000010)
#define ISP_REG_SW_CTL_SW_RST_P2_STATUS (0x00000020)
#define ISP_REG_SW_CTL_HW_RST_P2        (0x00000040)



/* if isp has been suspend, frame cnt needs to add previous value*/
#define ISP_RD32_TG_CAM_FRM_CNT(IrqType, reg_module) ({\
	unsigned int _regVal;\
	_regVal = ISP_RD32(CAM_REG_TG_INTER_ST(reg_module));\
	_regVal = ((_regVal & 0x00FF0000) >> 16) +\
		g_BkReg[IrqType].CAM_TG_INTER_ST;\
	if (_regVal > 255) { \
		_regVal -= 256;\
	} \
	_regVal;\
})

#if 0
#include "camera_isp_reg.c"
#include "camera_isp_isr.c"
#endif

/*******************************************************************************
 * Add MET ftrace event for power profilling. CAM_enter when SOF and CAM_leave
 * when P1_Done.
 ******************************************************************************/
#if defined(ISP_MET_READY)
int MET_Event_Get_BPP(enum _isp_dma_enum_ dmao, unsigned int reg_module)
{
	unsigned int fmt_sel = ISP_RD32(CAM_REG_CTL_FMT_SEL(reg_module));
	int ret = 0;

	if (dmao == _imgo_) {
		switch (fmt_sel & 0x1F0) {
		case 0:
		case 12:
		case 13:
		case 14:
		case 15:
			ret = 16;
			break;
		case 8:
			ret = 8;
			break;
		case 9:
		case 16:
			ret = 10;
			break;
		case 10:
			ret = 12;
			break;
		case 11:
			ret = 14;
			break;
		default:
			LOG_ERR("get imgo bpp error\n");
			break;
		}
	} else if (dmao == _rrzo_) {
		switch (fmt_sel & 0x0C) {
		case 0:
			ret = 8;
			break;
		case 1:
			ret = 10;
			break;
		case 2:
			ret = 12;
			break;
		default:
			LOG_ERR("get rrzo bpp error\n");
			break;
		}
	}

	return ret;
}

void MET_Events_Trace(bool enter, unsigned int reg_module)
{
	if (enter) {
		int imgo_en = 0, rrzo_en = 0, imgo_bpp, rrzo_bpp;
		int imgo_xsize, imgo_ysize;
		int rrzo_xsize, rrzo_ysize, rrz_src_w, rrz_src_h, rrz_dst_w;
		int rrz_dst_h, rrz_hori_step, rrz_vert_step;
		unsigned int dma_en, rrz_in, rrz_out;

		dma_en = ISP_RD32(CAM_REG_CTL_DMA_EN(reg_module));
		rrz_in = ISP_RD32(CAM_REG_RRZ_IN_IMG(reg_module));
		rrz_out = ISP_RD32(CAM_REG_RRZ_OUT_IMG(reg_module));
		imgo_en = dma_en & 0x1;
		rrzo_en = dma_en & 0x4;
		imgo_bpp = MET_Event_Get_BPP(_imgo_, reg_module);
		rrzo_bpp = MET_Event_Get_BPP(_rrzo_, reg_module);
		imgo_xsize = (int)(ISP_RD32(
				CAM_REG_IMGO_XSIZE(reg_module)) & 0xFFFF);

		imgo_ysize = (int)(ISP_RD32(
				CAM_REG_IMGO_YSIZE(reg_module)) & 0xFFFF);

		rrzo_xsize = (int)(ISP_RD32(
				CAM_REG_RRZO_XSIZE(reg_module)) & 0xFFFF);

		rrzo_ysize = (int)(ISP_RD32(
				CAM_REG_RRZO_YSIZE(reg_module)) & 0xFFFF);
		rrz_src_w = rrz_in & 0xFFFF;
		rrz_src_h = (rrz_in >> 16) & 0xFFFF;
		rrz_dst_w = rrz_out & 0xFFFF;
		rrz_dst_h = (rrz_out >> 16) & 0xFFFF;
		rrz_hori_step = (int)(ISP_RD32(
				CAM_REG_RRZ_HORI_STEP(reg_module)) & 0x3FFFF);

		rrz_vert_step = (int)(ISP_RD32(
				CAM_REG_RRZ_VERT_STEP(reg_module)) & 0x3FFFF);

		trace_ISP_Pass1_CAM_enter(imgo_en, rrzo_en, imgo_bpp,
			rrzo_bpp, imgo_xsize, imgo_ysize, rrzo_xsize,
			rrzo_ysize, rrz_src_w, rrz_src_h, rrz_dst_w,
			rrz_dst_h, rrz_hori_step, rrz_vert_step);
	} else {
		trace_ISP_Pass1_CAM_leave(0);
	}
}
#endif
/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int ISP_MsToJiffies(unsigned int Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int ISP_UsToJiffies(unsigned int Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}

/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int ISP_GetIRQState(unsigned int type,
	unsigned int stType, unsigned int userNumber, unsigned int stus)
{
	unsigned int ret;
	/* FIX to avoid build warning */
	unsigned long flags; /* old: unsigned int flags;*/

	/*  */
	spin_lock_irqsave(&(IspInfo.SpinLockIrq[type]), flags);
	ret = (IspInfo.IrqInfo.Status[type][stType][userNumber] & stus);
	spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[type]), flags);
	/*  */
	return ret;
}



/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int ISP_JiffiesToMs(unsigned int Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}

/*******************************************************************************
 *
 ******************************************************************************/

static void ISP_DumpDmaDeepDbg(enum ISP_IRQ_TYPE_ENUM module)
{
	unsigned int uni_path;
	unsigned int flk2_sel;
	unsigned int hds2_sel;
	unsigned int dmaerr[_cam_max_];
	enum ISP_DEV_NODE_ENUM regModule; /* for read/write register */

	switch (module) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
		regModule = ISP_CAM_A_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		regModule = ISP_CAM_B_IDX;
		break;
	default:
		LOG_ERR("unsupported module:0x%x\n", module);
		return;
	}


	dmaerr[_imgo_] =
		(unsigned int)ISP_RD32(CAM_REG_IMGO_ERR_STAT(regModule));
	dmaerr[_rrzo_] =
		(unsigned int)ISP_RD32(CAM_REG_RRZO_ERR_STAT(regModule));
	dmaerr[_aao_] = (unsigned int)ISP_RD32(CAM_REG_AAO_ERR_STAT(regModule));
	dmaerr[_afo_] = (unsigned int)ISP_RD32(CAM_REG_AFO_ERR_STAT(regModule));
	dmaerr[_lcso_] =
		(unsigned int)ISP_RD32(CAM_REG_LCSO_ERR_STAT(regModule));
	dmaerr[_ufeo_] =
		(unsigned int)ISP_RD32(CAM_REG_UFEO_ERR_STAT(regModule));
	dmaerr[_bpci_] =
		(unsigned int)ISP_RD32(CAM_REG_BPCI_ERR_STAT(regModule));
	dmaerr[_lsci_] =
		(unsigned int)ISP_RD32(CAM_REG_LSCI_ERR_STAT(regModule));
	dmaerr[_pdo_] = (unsigned int)ISP_RD32(CAM_REG_PDO_ERR_STAT(regModule));
	dmaerr[_pso_] = (unsigned int)ISP_RD32(CAM_REG_PSO_ERR_STAT(regModule));


	dmaerr[_eiso_] = (unsigned int)
		ISP_RD32(CAM_UNI_REG_EISO_ERR_STAT(ISP_UNI_A_IDX));
	dmaerr[_flko_] = (unsigned int)
		ISP_RD32(CAM_UNI_REG_FLKO_ERR_STAT(ISP_UNI_A_IDX));
	dmaerr[_rsso_] = (unsigned int)
		ISP_RD32(CAM_UNI_REG_RSSO_A_ERR_STAT(ISP_UNI_A_IDX));
	dmaerr[_rawi_] = (unsigned int)
		ISP_RD32(CAM_UNI_REG_RAWI_ERR_STAT(ISP_UNI_A_IDX));

	IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
		"mmsys:0x%x, imgsys:0x%x, camsys:0x%x",
		ISP_RD32(ISP_MMSYS_CONFIG_BASE + 0x100),
		ISP_RD32(ISP_IMGSYS_CONFIG_BASE),
		ISP_RD32(ISP_CAMSYS_CONFIG_BASE));

	uni_path = ISP_RD32(CAM_UNI_REG_TOP_PATH_SEL(ISP_UNI_A_IDX));
	flk2_sel = (uni_path & 0x3);
	hds2_sel = ((uni_path >> 8) & 0x3);
	switch (module) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
	"CAM_A:IMGO:0x%x,RRZO:0x%x,AAO=0x%x,AFO=0x%x,LCSO=0x%x,UFEO=0x%x,BPCI:0x%x,LSCI=0x%x,PDO=0x%x,PSO=0x%x\n",
			dmaerr[_imgo_],
			dmaerr[_rrzo_],
			dmaerr[_aao_],
			dmaerr[_afo_],
			dmaerr[_lcso_],
			dmaerr[_ufeo_],
			dmaerr[_bpci_],
			dmaerr[_lsci_],
			dmaerr[_pdo_],
			dmaerr[_pso_]);
		if (flk2_sel == 0) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAM_A+UNI: FLKO:0x%x, RAWI:0x%x\n",
				dmaerr[_flko_], dmaerr[_rawi_]);
		}
		if (hds2_sel == 0) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAM_A+UNI: EISO:0x%x, RSSO_A:0x%x, RAWI:0x%x\n",
				dmaerr[_eiso_], dmaerr[_rsso_], dmaerr[_rawi_]);
		}
		g_DmaErr_CAM[module][_imgo_] |= dmaerr[_imgo_];
		g_DmaErr_CAM[module][_rrzo_] |= dmaerr[_rrzo_];
		g_DmaErr_CAM[module][_aao_] |= dmaerr[_aao_];
		g_DmaErr_CAM[module][_afo_] |= dmaerr[_afo_];
		g_DmaErr_CAM[module][_lcso_] |= dmaerr[_lcso_];
		g_DmaErr_CAM[module][_ufeo_] |= dmaerr[_ufeo_];
		g_DmaErr_CAM[module][_bpci_] |= dmaerr[_bpci_];
		g_DmaErr_CAM[module][_lsci_] |= dmaerr[_lsci_];
		g_DmaErr_CAM[module][_pdo_] |= dmaerr[_pdo_];
		g_DmaErr_CAM[module][_pso_] |= dmaerr[_pso_];
		g_DmaErr_CAM[module][_flko_] |= dmaerr[_flko_];
		g_DmaErr_CAM[module][_eiso_] |= dmaerr[_eiso_];
		g_DmaErr_CAM[module][_rsso_] |= dmaerr[_rsso_];
		g_DmaErr_CAM[module][_rawi_] |= dmaerr[_rawi_];
		break;
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
	"CAM_B:IMGO:0x%x,RRZO:0x%x,AAO=0x%x,AFO=0x%x,LCSO=0x%x,UFEO=0x%x,BPCI:0x%x,LSCI=0x%x,PDO=0x%x,PSO=0x%x\n",
			dmaerr[_imgo_],
			dmaerr[_rrzo_],
			dmaerr[_aao_],
			dmaerr[_afo_],
			dmaerr[_lcso_],
			dmaerr[_ufeo_],
			dmaerr[_bpci_],
			dmaerr[_lsci_],
			dmaerr[_pdo_],
			dmaerr[_pso_]);
		if (flk2_sel == 1) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAM_B+UNI: FLKO:0x%x, RAWI:0x%x\n",
				dmaerr[_flko_], dmaerr[_rawi_]);
		}
		if (hds2_sel == 1) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAM_B+UNI: EISO:0x%x, RSSO_A:0x%x, RAWI:0x%x\n",
				dmaerr[_eiso_], dmaerr[_rsso_], dmaerr[_rawi_]);
		}
		g_DmaErr_CAM[module][_imgo_] |= dmaerr[_imgo_];
		g_DmaErr_CAM[module][_rrzo_] |= dmaerr[_rrzo_];
		g_DmaErr_CAM[module][_aao_] |= dmaerr[_aao_];
		g_DmaErr_CAM[module][_afo_] |= dmaerr[_afo_];
		g_DmaErr_CAM[module][_lcso_] |= dmaerr[_lcso_];
		g_DmaErr_CAM[module][_ufeo_] |= dmaerr[_ufeo_];
		g_DmaErr_CAM[module][_bpci_] |= dmaerr[_bpci_];
		g_DmaErr_CAM[module][_lsci_] |= dmaerr[_lsci_];
		g_DmaErr_CAM[module][_pdo_] |= dmaerr[_pdo_];
		g_DmaErr_CAM[module][_pso_] |= dmaerr[_pso_];
		g_DmaErr_CAM[module][_flko_] |= dmaerr[_flko_];
		g_DmaErr_CAM[module][_eiso_] |= dmaerr[_eiso_];
		g_DmaErr_CAM[module][_rsso_] |= dmaerr[_rsso_];
		g_DmaErr_CAM[module][_rawi_] |= dmaerr[_rawi_];
		break;
	default:
		LOG_ERR("unsupported module:0x%x\n", module);
		break;
	}

	smi_debug_bus_hang_detect(SMI_PARAM_BUS_OPTIMIZATION, 1, 0, 1);
	/* smi_dumpDebugMsg(); */
}


#define RegDump(start, end) {\
	unsigned int i;\
	for (i = start; i <= end; i += 0x10) {\
		LOG_INF("QQ [0x%08X %08X],[0x%08X %08X],",\
			(unsigned int)(ISP_TPIPE_ADDR + i),\
			(unsigned int)ISP_RD32(ISP_ADDR + i),\
			(unsigned int)(ISP_TPIPE_ADDR + i+0x4),\
			(unsigned int)ISP_RD32(ISP_ADDR + i+0x4));\
		LOG_INF("[0x%08X %08X],[0x%08X %08X]\n",\
			(unsigned int)(ISP_TPIPE_ADDR + i+0x8),\
			(unsigned int)ISP_RD32(ISP_ADDR + i+0x8),\
			(unsigned int)(ISP_TPIPE_ADDR + i+0xc),\
			(unsigned int)ISP_RD32(ISP_ADDR + i+0xc));\
	} \
}


static int ISP_DumpSeninfReg(void)
{
	int Ret = 0;
	/*  */
	LOG_INF("- E.");
	/*Sensor interface Top mux and Package counter*/
	LOG_INF("seninf:0008(0x%x)-0010(0x%x)-0a10(0x%x)-1a10(0x%x)",
		ISP_RD32(ISP_SENINF0_BASE + 0x0008),
		ISP_RD32(ISP_SENINF0_BASE + 0x0010),
		ISP_RD32(ISP_SENINF0_BASE + 0x0a10),
		ISP_RD32(ISP_SENINF1_BASE + 0x0a10));

	LOG_INF("0a1c(0x%x)-1a1c(0x%x)-0a1c(0x%x)-1a1c(0x%x)\n",
		ISP_RD32(ISP_SENINF0_BASE + 0x0a1c),
		ISP_RD32(ISP_SENINF1_BASE + 0x0a1c),
		ISP_RD32(ISP_SENINF0_BASE + 0x0a1c),
		ISP_RD32(ISP_SENINF1_BASE + 0x0a1c));
	/*Sensor interface0 control*/
	LOG_INF("seninf:0200(0x%x)-0204(0x%x)-0a00(0x%x)-0a14(0x%x)",
		ISP_RD32(ISP_SENINF0_BASE + 0x0200),
		ISP_RD32(ISP_SENINF0_BASE + 0x0204),
		ISP_RD32(ISP_SENINF0_BASE + 0x0a00),
		ISP_RD32(ISP_SENINF0_BASE + 0x0a14));

	LOG_INF("0a3c(0x%x)-0a44(0x%x)-0af0(0x%x)-0af4(0x%x)\n",
		ISP_RD32(ISP_SENINF0_BASE + 0x0a3c),
		ISP_RD32(ISP_SENINF0_BASE + 0x0a44),
		ISP_RD32(ISP_SENINF0_BASE + 0x0af0),
		ISP_RD32(ISP_SENINF0_BASE + 0x0af4));
	/*Sensor interface1 control*/
	LOG_INF("seninf:1200(0x%x)-1204(0x%x)-1a00(0x%x)-1a14(0x%x)",
		ISP_RD32(ISP_SENINF1_BASE + 0x0200),
		ISP_RD32(ISP_SENINF1_BASE + 0x0204),
		ISP_RD32(ISP_SENINF1_BASE + 0x0a00),
		ISP_RD32(ISP_SENINF1_BASE + 0x0a14));

	LOG_INF("1a3c(0x%x)-1a44(0x%x)-1af0(0x%x)-1af4(0x%x)\n",
		ISP_RD32(ISP_SENINF1_BASE + 0x0a3c),
		ISP_RD32(ISP_SENINF1_BASE + 0x0a44),
		ISP_RD32(ISP_SENINF1_BASE + 0x0af0),
		ISP_RD32(ISP_SENINF1_BASE + 0x0af4));
	/*Sensor interface mux*/
	LOG_INF("seninf:0d00(0x%x)-0d08(0x%x)-0d14(0x%x)-0d18(0x%x)",
		ISP_RD32(ISP_SENINF0_BASE + 0x0d00),
		ISP_RD32(ISP_SENINF0_BASE + 0x0d08),
		ISP_RD32(ISP_SENINF0_BASE + 0x0d14),
		ISP_RD32(ISP_SENINF0_BASE + 0x0d18));

	LOG_INF("1d00(0x%x)-1d08(0x%x)-1d14(0x%x)-1d18(0x%x)\n",
		ISP_RD32(ISP_SENINF1_BASE + 0x0d00),
		ISP_RD32(ISP_SENINF1_BASE + 0x0d08),
		ISP_RD32(ISP_SENINF1_BASE + 0x0d14),
		ISP_RD32(ISP_SENINF1_BASE + 0x0d18));
	LOG_INF("seninf:2d00(0x%x)-2d08(0x%x)-2d14(0x%x)-2d18(0x%x)",
		ISP_RD32(ISP_SENINF2_BASE + 0x0d00),
		ISP_RD32(ISP_SENINF2_BASE + 0x0d08),
		ISP_RD32(ISP_SENINF2_BASE + 0x0d14),
		ISP_RD32(ISP_SENINF2_BASE + 0x0d18));

	LOG_INF("3d00(0x%x)-3d08(0x%x)-3d14(0x%x)-3d18(0x%x)\n",
		ISP_RD32(ISP_SENINF3_BASE + 0x0d00),
		ISP_RD32(ISP_SENINF3_BASE + 0x0d08),
		ISP_RD32(ISP_SENINF3_BASE + 0x0d14),
		ISP_RD32(ISP_SENINF3_BASE + 0x0d18));
	/* IMGPLL frequency */
	LOG_INF("IMGPLL frequency(0x%x)[HPM:0x114EC5, LPM:0xC8000]",
		ISP_RD32(CLOCK_CELL_BASE + 0x0264));

	/*  */
	return Ret;

}
static int ISP_DumpReg(void)
{
	int Ret = 0;

#if 0
	/*  */
	/* spin_lock_irqsave(&(IspInfo.SpinLock), flags); */

	/* tile tool parse range */
	/* #define ISP_ADDR_START  0x15004000 */
	/* #define ISP_ADDR_END    0x15006000 */
	/*  */
	/* N3D control */
	ISP_WR32((ISP_ADDR + 0x40c0), 0x746);
	LOG_DBG("[0x%08X %08X] [0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x40c0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x40c0),
		(unsigned int)(ISP_TPIPE_ADDR + 0x40d8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x40d8));
	ISP_WR32((ISP_ADDR + 0x40c0), 0x946);
	LOG_DBG("[0x%08X %08X] [0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x40c0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x40c0),
		(unsigned int)(ISP_TPIPE_ADDR + 0x40d8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x40d8));

	/* isp top */
	RegDump(0x0, 0x200);
	/* dump p1 dma reg */
	RegDump(0x3200, 0x3570);
	/* dump all isp dma reg */
	RegDump(0x3300, 0x3400);
	/* dump all isp dma err reg */
	RegDump(0x3560, 0x35e0);
	/* TG1 */
	RegDump(0x410, 0x4a0);
	/* TG2 */
	RegDump(0x2410, 0x2450);
	/* hbin */
	LOG_ERR("[0x%08X %08X],[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4f0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x534),
		(unsigned int)(ISP_TPIPE_ADDR + 0x4f4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x538));
	/* LSC */
	RegDump(0x530, 0x550);
	/* awb win */
	RegDump(0x5b0, 0x5d0);
	/* ae win */
	RegDump(0x650, 0x690);
	/* af win */
	RegDump(0x6b0, 0x700);
	/* flk */
	RegDump(0x770, 0x780);
	/* rrz */
	RegDump(0x7a0, 0x7d0);
	/* eis */
	RegDump(0xdc0, 0xdf0);
	/* dmx/rmx/bmx */
	RegDump(0xe00, 0xe30);
	/* Mipi source */
	LOG_DBG("[0x%08X %08X],[0x%08X %08X]",
		(unsigned int)(0x10217000),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR),
		(unsigned int)(0x10217004),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0x4));
	LOG_DBG("[0x%08X %08X],[0x%08X %08X]",
		(unsigned int)(0x10217008),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0x8),
		(unsigned int)(0x1021700c),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0xc));
	LOG_DBG("[0x%08X %08X],[0x%08X %08X]",
		(unsigned int)(0x10217030),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0x30),
		(unsigned int)(0x10219030),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0x2030));

	/* NSCI2 1 debug */
	ISP_WR32((ISP_ADDR + 0x43B8), 0x02);
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x43B8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43B8));
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x43BC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43BC));
	ISP_WR32((ISP_ADDR + 0x43B8), 0x12);
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x43B8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43B8));
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x43BC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43BC));
	/* NSCI2 3 debug */
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x02);
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43BC));
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x12);
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x43B8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BBC));

	/* seninf1 */
	LOG_ERR("[0x%08X %08X],[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4008),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4008),
		(unsigned int)(ISP_TPIPE_ADDR + 0x4100),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4100));
	RegDump(0x4120, 0x4160);
	RegDump(0x4360, 0x43f0)
	/* seninf2 */
	LOG_ERR("[0x%08X %08X],[0x%08X %08X]",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4008),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4008),
		(unsigned int)(ISP_TPIPE_ADDR + 0x4100),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4100));
	RegDump(0x4520, 0x4560);
	RegDump(0x4600, 0x4610);
	RegDump(0x4760, 0x47f0);
	/* LSC_D */
	RegDump(0x2530, 0x2550);
	/* awb_d */
	RegDump(0x25b0, 0x25d0);
	/* ae_d */
	RegDump(0x2650, 0x2690);
	/* af_d */
	RegDump(0x26b0, 0x2700);
	/* rrz_d */
	RegDump(0x27a0, 0x27d0);
	/* rmx_d/bmx_d/dmx_d */
	RegDump(0x2e00, 0x2e30);

	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x800),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x800));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x880),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x880));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x884),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x884));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x888),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x888));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x8A0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x8A0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x920),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x920));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x924),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x924));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x928),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x928));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x92C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x92C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x930),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x930));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x934),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x934));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x938),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x938));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x93C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x93C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x960),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x960));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x9C4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x9C4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x9E4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x9E4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x9E8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x9E8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x9EC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x9EC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA00),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA00));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA04),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA04));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA08),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA08));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA0C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA0C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA10),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA10));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA14),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA14));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xA20),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xA20));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xAA0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xAA0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xACC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xACC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB00),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB00));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB04),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB04));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB08),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB08));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB0C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB0C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB10),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB10));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB14),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB14));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB18),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB18));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB1C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB1C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB20),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB20));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB44),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB44));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB48),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB48));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB4C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB4C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB50),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB50));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB54),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB54));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB58),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB58));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB5C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB5C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xB60),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xB60));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBA0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBA0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBA4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBA4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBA8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBA8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBAC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBAC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBB0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBB0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBB4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBB4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBBC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xBC0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xBC0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xC20),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xC20));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCC0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCC0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCE4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCE4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCE8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCE8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCEC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCEC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCF0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCF0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCF4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCF4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCF8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCF8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xCFC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xCFC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD24),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD24));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD28),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD28));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD2C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD2c));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD40),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD40));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD64),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD64));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD68),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD68));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD6C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD6c));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD70),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD70));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD74),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD74));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD78),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD78));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xD7C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xD7C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xDA4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xDA4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xDA8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xDA8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0xDAC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xDAC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2410),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2410));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2414),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2414));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2418),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2418));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x241C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x241C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2420),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2420));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x243C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x243C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2440),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2440));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2444),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2444));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x2448),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x2448));

	/* seninf3 */
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4900),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4900));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4920),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4920));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4924),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4924));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4928),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4928));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x492C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x492C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4930),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4930));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4934),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4934));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4938),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4938));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BA0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BA0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BA4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BA4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BA8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BA8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BAC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BAC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BB0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BB4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB4));
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x10);
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BBC));
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x11);
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BBC));
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x12);
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BBC));
	/* seninf4 */
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D00),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D00));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D20),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D20));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D24),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D24));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D28),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D28));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D2C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D2C));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D30),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D30));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D34),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D34));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4D38),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4D38));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FA0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FA0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FA4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FA4));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FA8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FA8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FAC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FAC));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FB0),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FB0));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FB4),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FB4));
	ISP_WR32((ISP_ADDR + 0x4FB8), 0x10);
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FBC));
	ISP_WR32((ISP_ADDR + 0x4FB8), 0x11);
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FBC));
	ISP_WR32((ISP_ADDR + 0x4FB8), 0x12);
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FB8));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x4FBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4FBC));

	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_TPIPE_ADDR + 0x35FC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x35FC));
	LOG_DBG("end MT6593");

	/*  */
	LOG_DBG("0x%08X %08X ",
		(unsigned int)ISP_ADDR_CAMINF,
		(unsigned int)ISP_RD32(ISP_ADDR_CAMINF));
	LOG_DBG("0x%08X %08X ",
		(unsigned int)(ISP_TPIPE_ADDR + 0x150),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x150));
	/*  */
	/* debug msg for direct link */


	/* mdp crop */
	LOG_DBG("MDPCROP Related");
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_ADDR + 0xd10),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xd10));
	LOG_DBG("0x%08X %08X",
		(unsigned int)(ISP_ADDR + 0xd20),
		(unsigned int)ISP_RD32(ISP_ADDR + 0xd20));
	/* cq */
	LOG_DBG("CQ Related");
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x6000);
	LOG_DBG("0x%08X %08X (0x15004160=6000)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x7000);
	LOG_DBG("0x%08X %08X (0x15004160=7000)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x8000);
	LOG_DBG("0x%08X %08X (0x15004160=8000)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	/* imgi_debug */
	LOG_DBG("IMGI_DEBUG Related");
	ISP_WR32(ISP_IMGSYS_BASE + 0x75f4, 0x001e);
	LOG_DBG("0x%08X %08X (0x150075f4=001e)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x75f4, 0x011e);
	LOG_DBG("0x%08X %08X (0x150075f4=011e)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x75f4, 0x021e);
	LOG_DBG("0x%08X %08X (0x150075f4=021e)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x75f4, 0x031e);
	LOG_DBG("0x%08X %08X (0x150075f4=031e)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	/* yuv */
	LOG_DBG("yuv-mdp crop Related");
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x3014);
	LOG_DBG("0x%08X %08X (0x15004160=3014)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	LOG_DBG("yuv-c24b out Related");
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x301e);
	LOG_DBG("0x%08X %08X (0x15004160=301e)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x301f);
	LOG_DBG("0x%08X %08X (0x15004160=301f)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x3020);
	LOG_DBG("0x%08X %08X (0x15004160=3020)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));
	ISP_WR32(ISP_IMGSYS_BASE + 0x4160, 0x3021);
	LOG_DBG("0x%08X %08X (0x15004160=3021)",
		(unsigned int)(ISP_IMGSYS_BASE + 0x4164),
		(unsigned int)ISP_RD32(ISP_IMGSYS_BASE + 0x4164));


#if 0 /* _mt6593fpga_dvt_use_ */
	{
		int tpipePA = ISP_RD32(ISP_ADDR + 0x204);
		int ctlStart = ISP_RD32(ISP_ADDR + 0x000);
		int ctlTcm = ISP_RD32(ISP_ADDR + 0x054);
		int map_va = 0, map_size;
		int i;
		int *pMapVa;
#define TPIPE_DUMP_SIZE    200

		if ((ctlStart & 0x01) && (tpipePA) &&
			(ctlTcm & 0x80000000)) {
			map_va = 0;
			m4u_mva_map_kernel(tpipePA, TPIPE_DUMP_SIZE, 0,
				&map_va, &map_size);
			pMapVa = map_va;
			LOG_DBG("pMapVa(0x%x),map_size(0x%x)",
					pMapVa, map_size);
			LOG_DBG("ctlStart(0x%x),tpipePA(0x%x),ctlTcm(0x%x)",
				ctlStart, tpipePA, ctlTcm);
			if (pMapVa) {
				for (i = 0; i < TPIPE_DUMP_SIZE; i += 10) {
					LOG_DBG(
						"[idx(%d)]%08X-%08X-%08X-%08X-%08X-",
						i, pMapVa[i],
						pMapVa[i + 1], pMapVa[i + 2],
						pMapVa[i + 3], pMapVa[i + 4]);
					LOG_DBG("%08X-%08X-%08X-%08X-%08X\n",
						pMapVa[i + 5], pMapVa[i + 6],
						pMapVa[i + 7], pMapVa[i + 8],
						pMapVa[i + 9]);
				}
			}
			m4u_mva_unmap_kernel(tpipePA, map_size, map_va);
		}
	}
#endif

	/* spin_unlock_irqrestore(&(IspInfo.SpinLock), flags); */
	/*  */
#endif
	LOG_DBG("- X.");
	/*  */
	return Ret;
}

static int ISP_DumpDIPReg(void)
{
	int Ret = 0;
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
#define TMP_STR1 "g_bDumpPhyISPBuf:(0x%x), g_pPhyISPBuffer:(0x%p),"
#define TMP_STR2 " g_bIonBuf:(0x%x)\n"

	LOG_INF(TMP_STR1 TMP_STR2, g_bDumpPhyISPBuf,
	g_pPhyISPBuffer, g_bIonBufferAllocated);
#undef TMP_STR1
#undef TMP_STR2

#ifdef AEE_DUMP_REDUCE_MEMORY
	if (g_bDumpPhyISPBuf == MFALSE) {
		ctrl_start = ISP_RD32(ISP_DIP_A_BASE + 0x0000);
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pPhyISPBuffer != NULL) {
				LOG_DBG("g_pPhyISPBuffer is not NULL:(0x%pK)\n",
					g_pPhyISPBuffer);
				vfree(g_pPhyISPBuffer);
				g_pPhyISPBuffer = NULL;
			}
			g_pPhyISPBuffer = vmalloc(ISP_DIP_REG_SIZE);
			if (g_pPhyISPBuffer == NULL)
				LOG_DBG("ERR: g_pPhyISPBuffer kmalloc fail\n");

			if (g_pKWTpipeBuffer != NULL) {
				LOG_DBG("g_pKWTpipeBuffer not NULL:(0x%pK)\n",
					g_pKWTpipeBuffer);
				vfree(g_pKWTpipeBuffer);
				g_pKWTpipeBuffer = NULL;
			}
			g_pKWTpipeBuffer = vmalloc(MAX_ISP_TILE_TDR_HEX_NO);
			if (g_pKWTpipeBuffer == NULL)
				LOG_DBG("ERR: g_pKWTpipeBuffer kmalloc fail\n");

			if (g_pKWCmdqBuffer != NULL) {
				LOG_DBG("g_KWCmdqBuffer is not NULL:(0x%pK)\n",
					g_pKWCmdqBuffer);
				vfree(g_pKWCmdqBuffer);
				g_pKWCmdqBuffer = NULL;
			}
			g_pKWCmdqBuffer = vmalloc(MAX_ISP_CMDQ_BUFFER_SIZE);
			if (g_pKWCmdqBuffer == NULL)
				LOG_DBG("ERROR: g_KWCmdqBuffer kmalloc fail\n");

			if (g_pKWVirISPBuffer != NULL) {
				LOG_DBG("g_KWVirISPBuffer not NULL:(0x%pK)\n",
					g_pKWVirISPBuffer);
				vfree(g_pKWVirISPBuffer);
				g_pKWVirISPBuffer = NULL;
			}
			g_pKWVirISPBuffer = vmalloc(ISP_DIP_REG_SIZE);
			if (g_pKWVirISPBuffer == NULL)
				LOG_DBG("ERR: g_KWVirISPBuffer kmalloc fail\n");
		}

		if (g_pPhyISPBuffer != NULL) {
			for (i = 0; i < (ISP_DIP_PHYSICAL_REG_SIZE >> 2);
				i = i + 4) {
				g_pPhyISPBuffer[i] =
					ISP_RD32(ISP_DIP_A_BASE + (i*4));
				g_pPhyISPBuffer[i+1] =
					ISP_RD32(ISP_DIP_A_BASE + ((i+1)*4));
				g_pPhyISPBuffer[i+2] =
					ISP_RD32(ISP_DIP_A_BASE + ((i+2)*4));
				g_pPhyISPBuffer[i+3] =
					ISP_RD32(ISP_DIP_A_BASE + ((i+3)*4));
			}
		} else {
			LOG_INF("g_pPhyISPBuffer:(0x%pK)\n", g_pPhyISPBuffer);
		}
		/* 0x15022204 */
		g_dumpInfo.tdri_baseaddr = ISP_RD32(ISP_DIP_A_BASE + 0x204);
		/* 0x15022400 */
		g_dumpInfo.imgi_baseaddr = ISP_RD32(ISP_DIP_A_BASE + 0x400);
		/* 0x15022520 */
		g_dumpInfo.dmgi_baseaddr = ISP_RD32(ISP_DIP_A_BASE + 0x520);
		g_tdriaddr = g_dumpInfo.tdri_baseaddr;
		for (cmdqidx = 0; cmdqidx < 32 ; cmdqidx++) {
			if (ctrl_start & (0x1<<cmdqidx)) {
				g_cmdqaddr = ISP_RD32(
					ISP_DIP_A_BASE + 0x108 + (cmdqidx*12));
				break;
			}
		}
		if ((g_TpipeBaseAddrInfo.MemPa != 0) &&
			(g_TpipeBaseAddrInfo.MemVa != NULL)
			&& (g_pKWTpipeBuffer != NULL)) {
			/* to get frame tdri baseaddress, otherwise you
			 * possible get one of the tdr bade addr
			 */
			offset = ((g_tdriaddr &
				(~(g_TpipeBaseAddrInfo.MemSizeDiff-1))) -
				g_TpipeBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_TpipeBaseAddrInfo.MemVa) +
				offset;
			if (copy_from_user(g_pKWTpipeBuffer,
				(void __user *)(OffsetAddr),
				MAX_ISP_TILE_TDR_HEX_NO) != 0) {
#define ERR_STR1 "cpy tpipe fail. tdriaddr:0x%x, MemVa:0x%lx,"
#define ERR_STR2 "MemPa:0x%x, offset:0x%x\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					g_tdriaddr,
					(uintptr_t)g_TpipeBaseAddrInfo.MemVa,
					g_TpipeBaseAddrInfo.MemPa, offset);
#undef ERR_STR1
#undef ERR_STR2
			}
		}
#define TMP_STR1 "tdraddr:0x%x, MemVa:0x%lx, MemPa:0x%x, MemSizeDiff:0x%x,"
#define TMP_STR2 " offset:0x%x, g_pKWTpipeBuffer:0x%pK\n"
		LOG_INF(TMP_STR1 TMP_STR2,
			g_tdriaddr, (uintptr_t)g_TpipeBaseAddrInfo.MemVa,
			g_TpipeBaseAddrInfo.MemPa,
			g_TpipeBaseAddrInfo.MemSizeDiff,
			offset, g_pKWTpipeBuffer);
#undef TMP_STR1
#undef TMP_STR2
		if ((g_CmdqBaseAddrInfo.MemPa != 0) &&
			(g_CmdqBaseAddrInfo.MemVa != NULL) &&
			(g_pKWCmdqBuffer != NULL) &&
			(g_pKWVirISPBuffer != NULL)) {
			offset = (g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa) +
				(g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			if (copy_from_user(g_pKWCmdqBuffer,
				(void __user *)(OffsetAddr),
				MAX_ISP_CMDQ_BUFFER_SIZE) != 0) {
#define ERR_STR1 "cpy cmdq fail. cmdqaddr:0x%x, MemVa:0x%lx,"
#define ERR_STR2 " MemPa:0x%x, offset:0x%x\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					g_cmdqaddr,
					(uintptr_t)g_CmdqBaseAddrInfo.MemVa,
					g_CmdqBaseAddrInfo.MemPa, offset);
#undef ERR_STR1
#undef ERR_STR2
			}
#define TMP_STR1 "cmdqidx:0x%x, cmdqaddr:0x%x, MemVa:0x%lx,"
#define TMP_STR2 " MemPa:0x%x, offset:0x%x\n"
			LOG_INF(TMP_STR1 TMP_STR2,
				cmdqidx, g_cmdqaddr,
				(uintptr_t)g_CmdqBaseAddrInfo.MemVa,
				g_CmdqBaseAddrInfo.MemPa, offset);
#undef TMP_STR1
#undef TMP_STR2
			offset = offset+g_CmdqBaseAddrInfo.MemSizeDiff;
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa) +
				offset;
			if (copy_from_user(g_pKWVirISPBuffer,
				(void __user *)(OffsetAddr),
				ISP_DIP_REG_SIZE) != 0) {
#define ERR_STR1 "cpy vir isp fail. cmdqaddr:0x%x, MVa:0x%lx, MPa:0x%x,"
#define ERR_STR2 " MSzDiff:0x%x, offset:0x%x\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
				g_cmdqaddr, (uintptr_t)g_CmdqBaseAddrInfo.MemVa,
				g_CmdqBaseAddrInfo.MemPa,
				g_CmdqBaseAddrInfo.MemSizeDiff, offset);
#undef ERR_STR1
#undef ERR_STR2
			}

			LOG_INF(
			"cmdqaddr:0x%x, MVa:0x%lx,MPa:0x%x,MSzDiff:0x%x\n",
			g_cmdqaddr,
			(uintptr_t)g_CmdqBaseAddrInfo.MemVa,
			g_CmdqBaseAddrInfo.MemPa,
			g_CmdqBaseAddrInfo.MemSizeDiff);

			LOG_INF("ofset:0x%x,KWCmdBuf:0x%pK,KWTdrBuf:0x%pK\n",
				offset, g_pKWCmdqBuffer, g_pKWTpipeBuffer);
		} else {
			LOG_INF("cmdqadd:0x%x,MVa:0x%lx,MPa:0x%x,",
				g_cmdqaddr,
				(uintptr_t)g_CmdqBaseAddrInfo.MemVa,
				g_CmdqBaseAddrInfo.MemPa);
			LOG_INF("MSzDiff:0x%x,KWCmdBuf:0x%pK,KWTdrBuf:0x%pK\n",
				g_CmdqBaseAddrInfo.MemSizeDiff,
				g_pKWCmdqBuffer, g_pKWTpipeBuffer);
		}
		g_bDumpPhyISPBuf = MTRUE;
	}

#else
		if (g_bDumpPhyISPBuf == MFALSE) {
			ctrl_start = ISP_RD32(ISP_DIP_A_BASE + 0x0000);
			if (g_pPhyISPBuffer != NULL) {
				for (i = 0;
					i < (ISP_DIP_PHYSICAL_REG_SIZE >> 2);
					i = i + 4) {
					g_pPhyISPBuffer[i] =
						ISP_RD32(ISP_DIP_A_BASE +
						(i*4));
					g_pPhyISPBuffer[i+1] =
						ISP_RD32(ISP_DIP_A_BASE +
						((i+1)*4));
					g_pPhyISPBuffer[i+2] =
						ISP_RD32(ISP_DIP_A_BASE +
						((i+2)*4));
					g_pPhyISPBuffer[i+3] =
						ISP_RD32(ISP_DIP_A_BASE +
						((i+3)*4));
			}
		} else {
			LOG_INF("g_pPhyISPBuffer:(0x%pK)\n", g_pPhyISPBuffer);
		}
		/* 0x15022204 */
		g_dumpInfo.tdri_baseaddr = ISP_RD32(ISP_DIP_A_BASE + 0x204);
		/* 0x15022400 */
		g_dumpInfo.imgi_baseaddr = ISP_RD32(ISP_DIP_A_BASE + 0x400);
		/* 0x15022520 */
		g_dumpInfo.dmgi_baseaddr = ISP_RD32(ISP_DIP_A_BASE + 0x520);
		g_tdriaddr = g_dumpInfo.tdri_baseaddr;
		for (cmdqidx = 0; cmdqidx < 32 ; cmdqidx++) {
			if (ctrl_start & (0x1<<cmdqidx)) {
				g_cmdqaddr =
					ISP_RD32(ISP_DIP_A_BASE +
					0x108 + (cmdqidx*12));
				break;
			}
		}
		if ((g_TpipeBaseAddrInfo.MemPa != 0) &&
			(g_TpipeBaseAddrInfo.MemVa != NULL)) {
			/* to get frame tdri baseaddress*/
			/* otherwide you possible get one of the tdr bade addr*/
			offset = ((g_tdriaddr &
				(~(g_TpipeBaseAddrInfo.MemSizeDiff-1)))
				-g_TpipeBaseAddrInfo.MemPa);
			OffsetAddr =
				((uintptr_t)g_TpipeBaseAddrInfo.MemVa)+offset;
			if (copy_from_user(g_KWTpipeBuffer,
				(void __user *)(OffsetAddr),
				MAX_ISP_TILE_TDR_HEX_NO) != 0) {
				LOG_ERR(
					"cpy tpipe fail.tdriaddr:0x%x,MVa:0x%lx",
					g_tdriaddr,
					(uintptr_t)g_TpipeBaseAddrInfo.MemVa);
				LOG_ERR("MPa:0x%x,ofset:0x%x\n",
					g_TpipeBaseAddrInfo.MemPa, offset);
			}
			LOG_INF("tdriaddr:0x%x, MemVa:0x%lx,",
				g_tdriaddr,
				(uintptr_t)g_TpipeBaseAddrInfo.MemVa);
			LOG_INF("MemPa:0x%x, MemSizeDiff:0x%x, offset:0x%x\n",
				g_TpipeBaseAddrInfo.MemPa,
				g_TpipeBaseAddrInfo.MemSizeDiff, offset);
		}
		if ((g_CmdqBaseAddrInfo.MemPa != 0) &&
			(g_CmdqBaseAddrInfo.MemVa != NULL)) {
			offset = (g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			OffsetAddr = ((uintptr_t)g_CmdqBaseAddrInfo.MemVa) +
				(g_cmdqaddr-g_CmdqBaseAddrInfo.MemPa);
			if (copy_from_user(g_KWCmdqBuffer,
				(void __user *)(OffsetAddr),
				MAX_ISP_CMDQ_BUFFER_SIZE) != 0) {
				LOG_ERR("cpy cmdq fail. cmdqaddr:0x%x,",
					g_cmdqaddr);
				LOG_ERR("MemVa:0x%lx,MemPa:0x%x,offset:0x%x\n",
					(uintptr_t)g_CmdqBaseAddrInfo.MemVa,
					g_CmdqBaseAddrInfo.MemPa,
					offset);
			}
			LOG_INF("cmdqidx:0x%x, cmdqaddr:0x%x, MemVa:0x%lx,".
				cmdqidx,
				g_cmdqaddr,
				(uintptr_t)g_CmdqBaseAddrInfo.MemVa);
			LOG_INF("MemPa:0x%x, offset:0x%x\n",
				g_CmdqBaseAddrInfo.MemPa, offset);
			offset = offset+g_CmdqBaseAddrInfo.MemSizeDiff;
			OffsetAddr =
				((uintptr_t)g_CmdqBaseAddrInfo.MemVa)+offset;
			if (copy_from_user(g_KWVirISPBuffer,
				(void __user *)(OffsetAddr),
				ISP_DIP_REG_SIZE) != 0) {
				LOG_ERR("cpy vir isp fail.cmdqaddr:0x%x,",
					g_cmdqaddr);
				LOG_ERR("MVa:0x%lx,MPa:0x%x,",
					(uintptr_t)g_CmdqBaseAddrInfo.MemVa,
					g_CmdqBaseAddrInfo.MemPa);
				LOG_ERR("MSzDiff:0x%x,ofset:0x%x\n",
					g_CmdqBaseAddrInfo.MemSizeDiff,
					offset);
			}
			LOG_INF("cmdqaddr:0x%x, MemVa:0x%lx,",
				g_cmdqaddr,
				(uintptr_t)g_CmdqBaseAddrInfo.MemVa);
			LOG_INF("MemPa:0x%x, MemSizeDiff:0x%x, offset:0x%x\n",
				g_CmdqBaseAddrInfo.MemPa,
				g_CmdqBaseAddrInfo.MemSizeDiff, offset);
		}
		g_bDumpPhyISPBuf = MTRUE;
	}
#endif

	LOG_INF("direct link:15020030(0x%x),g_bDumpPhyISPBuf:(0x%x),",
		ISP_RD32(ISP_IMGSYS_CONFIG_BASE + 0x0030),
		g_bDumpPhyISPBuf);
	LOG_INF("cmdqidx(0x%x),cmdqaddr(0x%x),tdriaddr(0x%x)\n",
		cmdqidx, g_cmdqaddr, g_tdriaddr);
	LOG_INF("isp: 15022000(0x%x)-15022004(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0000),
		ISP_RD32(ISP_DIP_A_BASE + 0x0004));
	LOG_INF("15022008(0x%x)-1502200C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0008),
		ISP_RD32(ISP_DIP_A_BASE + 0x000C));
	LOG_INF("isp: 15022010(0x%x)-15022014(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0010),
		ISP_RD32(ISP_DIP_A_BASE + 0x0014));
	LOG_INF("15022018(0x%x)-1502201C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0018),
		ISP_RD32(ISP_DIP_A_BASE + 0x001C));

	LOG_INF("isp: 15022D20(0x%x)-15022D24(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0D20),
		ISP_RD32(ISP_DIP_A_BASE + 0x0D24));

	LOG_INF("isp: 15022408(0x%x)-15022204(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0408),
		ISP_RD32(ISP_DIP_A_BASE + 0x0204));
	LOG_INF("15022208(0x%x)-1502220C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0208),
		ISP_RD32(ISP_DIP_A_BASE + 0x020C));
	LOG_INF("isp: 15022050(0x%x)-15022054(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0050),
		ISP_RD32(ISP_DIP_A_BASE + 0x0054));
	LOG_INF("15022058(0x%x)-1502205C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0058),
		ISP_RD32(ISP_DIP_A_BASE + 0x005C));
	LOG_INF("isp: 150220B8(0x%x)-150220BC(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x00B8),
		ISP_RD32(ISP_DIP_A_BASE + 0x00BC));
	LOG_INF("150220C0(0x%x)-150220C4(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x00C0),
		ISP_RD32(ISP_DIP_A_BASE + 0x00C4));
	LOG_INF("isp: 150220C8(0x%x)-150220CC(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x00C8),
		ISP_RD32(ISP_DIP_A_BASE + 0x00CC));
	LOG_INF("150220D0(0x%x)-150220D4(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x00D0),
		ISP_RD32(ISP_DIP_A_BASE + 0x00D4));
	LOG_INF("isp: 15022104(0x%x)-15022108(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0104),
		ISP_RD32(ISP_DIP_A_BASE + 0x0108));
	LOG_INF("1502211C(0x%x)-15022120(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x011C),
		ISP_RD32(ISP_DIP_A_BASE + 0x0120));
	LOG_INF("isp: 15022128(0x%x)-1502212C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0128),
		ISP_RD32(ISP_DIP_A_BASE + 0x012C));
	LOG_INF("15022134(0x%x)-15022138(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0134),
		ISP_RD32(ISP_DIP_A_BASE + 0x0138));
	LOG_INF("isp: 15022140(0x%x)-15022144(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0140),
		ISP_RD32(ISP_DIP_A_BASE + 0x0144));
	LOG_INF("1502214C(0x%x)-15022150(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x014C),
		ISP_RD32(ISP_DIP_A_BASE + 0x0150));
	LOG_INF("isp: 15022158(0x%x)-1502215C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0158),
		ISP_RD32(ISP_DIP_A_BASE + 0x015C));
	LOG_INF("15022164(0x%x)-15022168(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0164),
		ISP_RD32(ISP_DIP_A_BASE + 0x0168));
	LOG_INF("isp: 15022170(0x%x)-15022174(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0170),
		ISP_RD32(ISP_DIP_A_BASE + 0x0174));
	LOG_INF("1502217C(0x%x)-15022180(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x017C),
		ISP_RD32(ISP_DIP_A_BASE + 0x0180));
	LOG_INF("isp: 15022188(0x%x)-1502218C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0188),
		ISP_RD32(ISP_DIP_A_BASE + 0x018C));
	LOG_INF("15022194(0x%x)-15022198(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0194),
		ISP_RD32(ISP_DIP_A_BASE + 0x0198));
	LOG_INF("isp: 15022060(0x%x)-15022064(0x%x)-15022068(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0060),
		ISP_RD32(ISP_DIP_A_BASE + 0x0064),
		ISP_RD32(ISP_DIP_A_BASE + 0x0068));
		/* 0080, 0x15022080, DIP_A_CTL_DBG_SET */
		ISP_WR32(ISP_DIP_A_BASE + 0x80, 0x0);
		/* 06BC, 0x150226BC, DIP_A_DMA_DEBUG_SEL */
		ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x000013);
		/* 0084, 0x15022084, DIP_A_CTL_DBG_PORT */
	LOG_INF("0x000013 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x000113);
	LOG_INF("0x000113 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x000213);
	LOG_INF("0x000213 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x000313);
	LOG_INF("0x000313 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	/* IMG2O */
	/* 06BC, 0x150226BC, DIP_A_DMA_DEBUG_SEL */
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001013);
	/* 0084, 0x15022084, DIP_A_CTL_DBG_PORT */
	LOG_INF("0x001013 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001113);
	LOG_INF("0x001113 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001213);
	LOG_INF("0x001213 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001313);
	LOG_INF("0x001313 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	/*IMG3O */
	/* 06BC, 0x150226BC, DIP_A_DMA_DEBUG_SEL */
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001813);
	/* 0084, 0x15022084, DIP_A_CTL_DBG_PORT */
	LOG_INF("0x001813 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001913);
	LOG_INF("0x001913 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001A13);
	LOG_INF("0x001A13 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x6BC, 0x001B13);
	LOG_INF("0x001B13 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x080, 0x003016);
	LOG_INF("0x003016 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x080, 0x003017);
	LOG_INF("0x003017 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x080, 0x003018);
	LOG_INF("0x003018 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x080, 0x003019);
	LOG_INF("0x003019 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	ISP_WR32(ISP_DIP_A_BASE + 0x080, 0x005100);
	LOG_INF("0x005100 : isp: 0x15022084(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x084));
	/* DMA Error */
	LOG_INF("img2o  0x15022644(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x644));
	LOG_INF("img2bo 0x15022648(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x648));
	LOG_INF("img3o  0x1502264C(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x64C));
	LOG_INF("img3bo 0x15022650(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x650));
	LOG_INF("img3Co 0x15022654(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x654));
	LOG_INF("feo    0x15022658(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x658));
	LOG_INF("mfbo   0x1502265C(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x65C));
	LOG_INF("imgi   0x15022660(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x660));
	LOG_INF("imgbi  0x15022664(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x664));
	LOG_INF("imgci  0x15022668(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x668));
	LOG_INF("vipi   0x1502266c(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x66c));
	LOG_INF("vip2i  0x15022670(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x670));
	LOG_INF("vip3i  0x15022674(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x674));
	LOG_INF("dmgi   0x15022678(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x678));
	LOG_INF("depi   0x1502267c(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x67C));
	LOG_INF("lcei   0x15022680(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x680));
	LOG_INF("ufdi   0x15022684(0x%x)", ISP_RD32(ISP_DIP_A_BASE + 0x684));

	LOG_INF("CTL_INT_STATUSX      0x15022040(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x040));
	LOG_INF("CTL_CQ_INT_STATUSX   0x15022044(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x044));
	LOG_INF("CTL_CQ_INT2_STATUSX  0x15022048(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x048));
	LOG_INF("CTL_CQ_INT3_STATUSX  0x1502204C(0x%x)",
		ISP_RD32(ISP_DIP_A_BASE + 0x04C));
	LOG_INF("img3o: 0x15022290(0x%x)-0x15022298(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x290),
		ISP_RD32(ISP_DIP_A_BASE + 0x0298));
	LOG_INF("0x150222A0(0x%x)-0x150222A4(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x2A0),
		ISP_RD32(ISP_DIP_A_BASE + 0x02A4));
	LOG_INF("img3o: 0x150222A8(0x%x)-0x150222AC(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x02A8),
		ISP_RD32(ISP_DIP_A_BASE + 0x02AC));
	LOG_INF("0x150222B0(0x%x)-0x150222B4(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x02B0),
		ISP_RD32(ISP_DIP_A_BASE + 0x02B4));
	LOG_INF("img3o: 0x150222B8(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x02B8));

	LOG_INF("imgi: 0x15022400(0x%x)-0x15022408(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x400),
		ISP_RD32(ISP_DIP_A_BASE + 0x408));
	LOG_INF("0x15022410(0x%x)-0x15022414(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x410),
		ISP_RD32(ISP_DIP_A_BASE + 0x414));
	LOG_INF("imgi: 0x15022418(0x%x)-0x1502241C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x418),
		ISP_RD32(ISP_DIP_A_BASE + 0x41C));
	LOG_INF("0x15022420(0x%x)-0x15022424(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x420),
		ISP_RD32(ISP_DIP_A_BASE + 0x424));
	LOG_INF("mfbo: 0x15022350(0x%x)-0x15022358(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x350),
		ISP_RD32(ISP_DIP_A_BASE + 0x358));
	LOG_INF("0x15022360(0x%x)-0x15022364(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x360),
		ISP_RD32(ISP_DIP_A_BASE + 0x364));
	LOG_INF("mfbo: 0x15022368(0x%x)-0x1502236C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x368),
		ISP_RD32(ISP_DIP_A_BASE + 0x36C));
	LOG_INF("0x15022370(0x%x)-0x15022374(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x370),
		ISP_RD32(ISP_DIP_A_BASE + 0x374));
	LOG_INF("mfbo: 0x15022378(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x378));

	LOG_INF("img2o: 0x15022230(0x%x)-0x15022238(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x230),
		ISP_RD32(ISP_DIP_A_BASE + 0x238));
	LOG_INF("0x15022240(0x%x)-0x15022244(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x240),
		ISP_RD32(ISP_DIP_A_BASE + 0x244));
	LOG_INF("img2o: 0x15022248(0x%x)-0x1502224C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0248),
		ISP_RD32(ISP_DIP_A_BASE + 0x024C));
	LOG_INF("0x15022250(0x%x)-0x15022254(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0250),
		ISP_RD32(ISP_DIP_A_BASE + 0x0254));
	LOG_INF("img2o: 0x15022258(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0258));

	LOG_INF("lcei: 0x15022580(0x%x)-0x15022588(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x580),
		ISP_RD32(ISP_DIP_A_BASE + 0x588));
	LOG_INF("0x15022590(0x%x)-0x15022594(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x590),
		ISP_RD32(ISP_DIP_A_BASE + 0x594));
	LOG_INF("lcei: 0x15022598(0x%x)-0x1502259C(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0598),
		ISP_RD32(ISP_DIP_A_BASE + 0x059C));
	LOG_INF("0x150225A0(0x%x)-0x150225A4(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x05A0),
		ISP_RD32(ISP_DIP_A_BASE + 0x05A4));

	LOG_INF("crz: 0x15022C10(0x%x)-0x15022C14(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0xC10),
		ISP_RD32(ISP_DIP_A_BASE + 0xC14));
	LOG_INF("0x15022C18(0x%x)-0x15022C1C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0xC18),
		ISP_RD32(ISP_DIP_A_BASE + 0xC1C));
	LOG_INF("crz: 0x15022C20(0x%x)-0x15022C24(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0C20),
		ISP_RD32(ISP_DIP_A_BASE + 0x0C24));
	LOG_INF("0x15022C28(0x%x)-0x15022C2C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0C28),
		ISP_RD32(ISP_DIP_A_BASE + 0x0C2C));
	LOG_INF("crz: 0x15022C30(0x%x)-0x15022C34(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0C30),
		ISP_RD32(ISP_DIP_A_BASE + 0x0C34));
	LOG_INF("0x15022C38(0x%x)-0x15022C3C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0C38),
		ISP_RD32(ISP_DIP_A_BASE + 0x0C3C));
	LOG_INF("crz: 0x15022C40(0x%x)-0x15022C44(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0C40),
		ISP_RD32(ISP_DIP_A_BASE + 0x0C44));

	LOG_INF("mfb: 0x15022F60(0x%x)-0x15022F64(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0xF60),
		ISP_RD32(ISP_DIP_A_BASE + 0xF64));
	LOG_INF("0x15022F68(0x%x)-0x15022F6C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0xF68),
		ISP_RD32(ISP_DIP_A_BASE + 0xF6C));
	LOG_INF("mfb: 0x15022F70(0x%x)-0x15022F74(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0F70),
		ISP_RD32(ISP_DIP_A_BASE + 0x0F74));
	LOG_INF("0x15022F78(0x%x)-0x15022F7C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0F78),
		ISP_RD32(ISP_DIP_A_BASE + 0x0F7C));
	LOG_INF("mfb: 0x15022F80(0x%x)-0x15022F84(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0F80),
		ISP_RD32(ISP_DIP_A_BASE + 0x0F84));
	LOG_INF("0x15022F88(0x%x)-0x15022F8C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0F88),
		ISP_RD32(ISP_DIP_A_BASE + 0x0F8C));
	LOG_INF("mfb: 0x15022F90(0x%x)-0x15022F94(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0F90),
		ISP_RD32(ISP_DIP_A_BASE + 0x0F94));
	LOG_INF("0x15022F98(0x%x)-0x15022F9C(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0F98),
		ISP_RD32(ISP_DIP_A_BASE + 0x0F9C));
	LOG_INF("mfb: 0x15022FA0(0x%x)-0x15022FA4(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0FA0),
		ISP_RD32(ISP_DIP_A_BASE + 0x0FA4));
	LOG_INF("0x15022FA8(0x%x)-0x15022FAC(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0FA8),
		ISP_RD32(ISP_DIP_A_BASE + 0x0FAC));
	LOG_INF("mfb: 0x15022FB0(0x%x)-0x15022FB4(0x%x)-",
		ISP_RD32(ISP_DIP_A_BASE + 0x0FB0),
		ISP_RD32(ISP_DIP_A_BASE + 0x0FB4));
	LOG_INF("0x15022FB8(0x%x)\n",
		ISP_RD32(ISP_DIP_A_BASE + 0x0FB8));

	LOG_DBG("- X.");
	/*  */
	return Ret;
}

#ifndef CONFIG_MTK_CLKMGR /*CCF*/

static inline void Prepare_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order:
	 * CG_SCP_SYS_DIS-> CG_DISP0_SMI_COMMON -> CG_SCP_SYS_ISP/CAM ->
	 * ISP clk
	 */

	/* enable through smi API */
	smi_bus_prepare_enable(SMI_LARB1_REG_INDX, ISP_DEV_NAME, true);
	smi_bus_prepare_enable(SMI_LARB2_REG_INDX, ISP_DEV_NAME, true);


	ret = clk_prepare_enable(isp_clk.ISP_SCP_SYS_DIS);
	if (ret)
		LOG_ERR("cannot pre-en ISP_SCP_SYS_DIS clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot pre-en ISP_SCP_SYS_ISP clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_SCP_SYS_CAM);
	if (ret)
		LOG_ERR("cannot pre-en ISP_SCP_SYS_CAM clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_IMG_DIP);
	if (ret)
		LOG_ERR("cannot pre-en ISP_IMG_DIP clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSYS);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSYS clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMTG);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMTG clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_SENINF);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_SENINF clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSV0);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSV0 clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSV1);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSV1 clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSV2);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSV2 clock\n");
}

static inline void Disable_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order:
	 * ISP clk -> CG_SCP_SYS_ISP/CAM -> CG_DISP0_SMI_COMMON ->
	 * CG_SCP_SYS_DIS
	 */
	clk_disable_unprepare(isp_clk.ISP_IMG_DIP);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSYS);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMTG);
	clk_disable_unprepare(isp_clk.ISP_CAM_SENINF);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSV0);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSV1);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSV2);
	clk_disable_unprepare(isp_clk.ISP_SCP_SYS_CAM);
	clk_disable_unprepare(isp_clk.ISP_SCP_SYS_ISP);
	clk_disable_unprepare(isp_clk.ISP_SCP_SYS_DIS);

	smi_bus_disable_unprepare(SMI_LARB2_REG_INDX, ISP_DEV_NAME, true);
	smi_bus_disable_unprepare(SMI_LARB1_REG_INDX, ISP_DEV_NAME, true);
}

/* only for suspend/resume, disable isp cg but no MTCMOS*/
static inline void Prepare_Enable_cg_clock(void)
{
	int ret;

	LOG_INF("enable CG through SMI CLK API\n");
	smi_bus_prepare_enable(SMI_LARB2_REG_INDX, ISP_DEV_NAME, true);

	ret = clk_prepare_enable(isp_clk.ISP_IMG_DIP);
	if (ret)
		LOG_ERR("cannot pre-en ISP_IMG_DIP clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSYS);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSYS clock\n");

/*	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMTG);
 *	if (ret)
 *		LOG_ERR("cannot pre-en ISP_CAM_CAMTG clock\n");
 *
 *	ret = clk_prepare_enable(isp_clk.ISP_CAM_SENINF);
 *	if (ret)
 *		LOG_ERR("cannot pre-en ISP_CAM_SENINF clock\n");
 */

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSV0);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSV0 clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSV1);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSV1 clock\n");

	ret = clk_prepare_enable(isp_clk.ISP_CAM_CAMSV2);
	if (ret)
		LOG_ERR("cannot pre-en ISP_CAM_CAMSV2 clock\n");

}

static inline void Disable_Unprepare_cg_clock(void)
{
	clk_disable_unprepare(isp_clk.ISP_IMG_DIP);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSYS);
/*	clk_disable_unprepare(isp_clk.ISP_CAM_CAMTG);
 *	clk_disable_unprepare(isp_clk.ISP_CAM_SENINF);
 */
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSV0);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSV1);
	clk_disable_unprepare(isp_clk.ISP_CAM_CAMSV2);

	LOG_INF("disable CG through SMI CLK API\n");
	smi_bus_disable_unprepare(SMI_LARB2_REG_INDX, ISP_DEV_NAME, true);
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/

void ISP_Halt_Mask(unsigned int isphaltMask)
{
	unsigned int setReg;

	setReg = ISP_RD32(ISP_CAMSYS_CONFIG_BASE + 0x120) &
		~((unsigned int)(1 << (isphaltMask)));

	ISP_WR32(ISP_CAMSYS_CONFIG_BASE + 0x120, setReg);

	LOG_INF("ISP halt_en for dvfs:0x%x\n",
		ISP_RD32(ISP_CAMSYS_CONFIG_BASE + 0x120));
}
EXPORT_SYMBOL(ISP_Halt_Mask);

/*******************************************************************************
 *
 ******************************************************************************/
static void ISP_EnableClock(bool En)
{
#if defined(EP_NO_CLKMGR)
	unsigned int setReg;
#endif

	if (En) {
#if defined(EP_NO_CLKMGR) || defined(CONFIG_MTK_CLKMGR)
		spin_lock(&(IspInfo.SpinLockClock));

		switch (G_u4EnableClockCount) {
		case 0:
#ifdef EP_NO_CLKMGR  /*FPGA test*/
			/* Enable clock by hardcode:
			 * 1. CAMSYS_CG_CLR (0x1A000008) = 0xffffffff;
			 * 2. IMG_CG_CLR (0x15000008) = 0xffffffff;
			 */
			setReg = 0xFFFFFFFF;
			ISP_WR32(CAMSYS_REG_CG_CLR, setReg);
			ISP_WR32(IMGSYS_REG_CG_CLR, setReg);
#else /* mt6797 not support CLKMGR, only CCF!!*/
			/*LOG_INF("MTK_LEGACY:enable clk");*/
			enable_clock(MT_CG_DISP0_SMI_COMMON, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
			enable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
			enable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_SV, "CAMERA");
			/* enable_clock(MT_CG_IMAGE_FD, "CAMERA"); */
			enable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
#endif
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
			unsigned int _reg = ISP_RD32(CLOCK_CELL_BASE);

			ISP_WR32(CLOCK_CELL_BASE, _reg|(1<<6));
		}
		G_u4EnableClockCount++;
		spin_unlock(&(IspInfo.SpinLockClock));
		Prepare_Enable_ccf_clock(); /* !!cannot be used in spinlock!! */
#endif
	/* Disable CAMSYS_HALT1_EN: LSCI & BPCI, To avoid ISP halt keep arise */
		#if 0/* TBD */
	ISP_WR32(ISP_CAMSYS_CONFIG_BASE + 0x120, 0xFFFFFF4F);
		#endif
	} else {                /* Disable clock. */
#if defined(EP_NO_CLKMGR) || defined(CONFIG_MTK_CLKMGR)
		spin_lock(&(IspInfo.SpinLockClock));

		G_u4EnableClockCount--;
		switch (G_u4EnableClockCount) {
		case 0:
#ifdef EP_NO_CLKMGR

			/* Disable clock by hardcode:
			 * 1. CAMSYS_CG_SET (0x1A000004) = 0xffffffff;
			 * 2. IMG_CG_SET (0x15000004) = 0xffffffff;
			 */
			setReg = 0xFFFFFFFF;
			ISP_WR32(CAMSYS_REG_CG_SET, setReg);
			ISP_WR32(IMGSYS_REG_CG_SET, setReg);
#else
			/*LOG_INF("MTK_LEGACY:disable clk");*/
			/* do disable clock     */
			disable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
			disable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
			disable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
			disable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
			disable_clock(MT_CG_IMAGE_CAM_SV, "CAMERA");
			/* disable_clock(MT_CG_IMAGE_FD, "CAMERA"); */
			disable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
			disable_clock(MT_CG_DISP0_SMI_COMMON, "CAMERA");
#endif
			break;
		default:
			break;
		}
		spin_unlock(&(IspInfo.SpinLockClock));
#else
		/*LOG_INF("CCF:disable_unprepare clk\n");*/
		spin_lock(&(IspInfo.SpinLockClock));
		if (G_u4EnableClockCount == 0) {
			spin_unlock(&(IspInfo.SpinLockClock));
#define TMP_STR "G_u4EnableClockCount aleady be 0, do nothing\n"
			LOG_INF(TMP_STR);
#undef TMP_STR
			return;
		}

		G_u4EnableClockCount--;
		if (G_u4EnableClockCount == 0) {
			unsigned int _reg = ISP_RD32(CLOCK_CELL_BASE);

			ISP_WR32(CLOCK_CELL_BASE, _reg&(~(1<<6)));
		}
		spin_unlock(&(IspInfo.SpinLockClock));
		/* !!cannot be used in spinlock!! */
		Disable_Unprepare_ccf_clock();
#endif
	}
}



/*******************************************************************************
 *
 ******************************************************************************/
static inline void ISP_Reset(int module)
{
	/*    unsigned int Reg;*/
	/*    unsigned int setReg;*/

	LOG_DBG("- E.\n");
#define TMP_STR1 " Reset module(%d), CAMSYS clk gate(0x%x),"
#define TMP_STR2 " IMGSYS clk gate(0x%x)\n"
	LOG_DBG(TMP_STR1 TMP_STR2,
		module, ISP_RD32(CAMSYS_REG_CG_CON),
		ISP_RD32(IMGSYS_REG_CG_CON));
#undef TMP_STR1
#undef TMP_STR2

	switch (module) {
	case ISP_CAM_A_IDX:
	case ISP_CAM_B_IDX: {
		/* Reset CAM flow */
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x2);
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x1);
#if 0
		while (ISP_RD32(CAM_REG_CTL_SW_CTL(module)) != 0x2)
			LOG_DBG("CAM resetting... module:%d\n", module);
#else
		mdelay(1);
#endif
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x4);
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x0);

		break;
	}
	case ISP_CAMSV0_IDX:
	case ISP_CAMSV1_IDX:
	case ISP_CAMSV2_IDX:
	case ISP_CAMSV3_IDX:
	case ISP_CAMSV4_IDX:
	case ISP_CAMSV5_IDX: {
		/* Reset CAMSV flow */
		ISP_WR32(CAMSV_REG_SW_CTL(module), 0x4); /* SW_RST: 1 */
		ISP_WR32(CAMSV_REG_SW_CTL(module), 0x0); /* SW_RST: 0 */
		ISP_WR32(CAMSV_REG_SW_CTL(module), 0x1); /* IMGO_RST_TRIG: 1 */
		while (ISP_RD32(CAMSV_REG_SW_CTL(module)) != 0x3) {
			/* Polling IMGO_RST_ST to 1 */
			LOG_DBG("CAMSV resetting... module:%d\n", module);
		}
		ISP_WR32(CAMSV_REG_SW_CTL(module), 0x0); /* IMGO_RST_TRIG: 0 */

		break;
	}
	case ISP_UNI_A_IDX: {
		/* Reset UNI flow */
		ISP_WR32(CAM_UNI_REG_TOP_SW_CTL(module), 0x222);
		ISP_WR32(CAM_UNI_REG_TOP_SW_CTL(module), 0x111);
#if 0
		while (ISP_RD32(CAM_UNI_REG_TOP_SW_CTL(module)) != 0x222)
			LOG_DBG("UNI resetting... module:%d\n", module);
#else
		mdelay(1);
#endif
		ISP_WR32(CAM_UNI_REG_TOP_SW_CTL(module), 0x0444);
		ISP_WR32(CAM_UNI_REG_TOP_SW_CTL(module), 0x0);
		break;
	}
	case ISP_DIP_A_IDX: {

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
 ******************************************************************************/
static int ISP_ReadReg(struct ISP_REG_IO_STRUCT *pRegIo)
{
	unsigned int i;
	int Ret = 0;

	unsigned int module;
	void __iomem *regBase;


	/*  */
	struct ISP_REG_STRUCT reg;
	/* unsigned int* pData = (unsigned int*)pRegIo->Data; */
	struct ISP_REG_STRUCT *pData = (struct ISP_REG_STRUCT *)pRegIo->pData;

	module = pData->module;

	switch (module) {
	case ISP_CAM_A_IDX:
		regBase = ISP_CAM_A_BASE;
		break;
	case ISP_CAM_B_IDX:
		regBase = ISP_CAM_B_BASE;
		break;
	case ISP_CAMSV0_IDX:
		regBase = ISP_CAMSV0_BASE;
		break;
	case ISP_CAMSV1_IDX:
		regBase = ISP_CAMSV1_BASE;
		break;
	case ISP_CAMSV2_IDX:
		regBase = ISP_CAMSV2_BASE;
		break;
	case ISP_CAMSV3_IDX:
		regBase = ISP_CAMSV3_BASE;
		break;
	case ISP_CAMSV4_IDX:
		regBase = ISP_CAMSV4_BASE;
		break;
	case ISP_CAMSV5_IDX:
		regBase = ISP_CAMSV5_BASE;
		break;
	case ISP_DIP_A_IDX:
		regBase = ISP_DIP_A_BASE;
		break;
	case ISP_UNI_A_IDX:
		regBase = ISP_CAM_UNI_BASE;
		break;
	case 0xFF:
		regBase = ISP_SENINF0_BASE;
		break;
	case 0xFE:
		regBase = ISP_SENINF1_BASE;
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
		if (((regBase + reg.Addr) < (regBase + PAGE_SIZE))
			&& ((reg.Addr & 0x3) == 0)) {
			reg.Val = ISP_RD32(regBase + reg.Addr);
		} else {
			LOG_ERR("Wrong address(0x%lx)\n",
				(unsigned long)(regBase + reg.Addr));
			reg.Val = 0;
		}

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
 ******************************************************************************/
/* Note: Can write sensor's test model only, if need write to other modules,
 * need to modify current code flow.
 */
static int ISP_WriteRegToHw(
	struct ISP_REG_STRUCT *pReg,
	unsigned int         Count)
{
	int Ret = 0;
	unsigned int i;
	bool dbgWriteReg;
	unsigned int module;
	void __iomem *regBase;

	/* Use local variable to store IspInfo.DebugMask & ISP_DBG_WRITE_REG
	 * for saving lock time.
	 */
	spin_lock(&(IspInfo.SpinLockIsp));
	dbgWriteReg = IspInfo.DebugMask & ISP_DBG_WRITE_REG;
	spin_unlock(&(IspInfo.SpinLockIsp));

	module = pReg->module;


	switch (module) {
	case ISP_CAM_A_IDX:
		regBase = ISP_CAM_A_BASE;
		break;
	case ISP_CAM_B_IDX:
		regBase = ISP_CAM_B_BASE;
		break;
	case ISP_CAMSV0_IDX:
		regBase = ISP_CAMSV0_BASE;
		break;
	case ISP_CAMSV1_IDX:
		regBase = ISP_CAMSV1_BASE;
		break;
	case ISP_CAMSV2_IDX:
		regBase = ISP_CAMSV2_BASE;
		break;
	case ISP_CAMSV3_IDX:
		regBase = ISP_CAMSV3_BASE;
		break;
	case ISP_CAMSV4_IDX:
		regBase = ISP_CAMSV4_BASE;
		break;
	case ISP_CAMSV5_IDX:
		regBase = ISP_CAMSV5_BASE;
		break;
	case ISP_DIP_A_IDX:
		regBase = ISP_DIP_A_BASE;
		break;
	case ISP_UNI_A_IDX:
		regBase = ISP_CAM_UNI_BASE;
		break;
	case 0xFF:
		regBase = ISP_SENINF0_BASE;
		break;
	case 0xFE:
		regBase = ISP_SENINF1_BASE;
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
		if (dbgWriteReg) {
#define TMP_STR "module(%d), base(0x%lx), Addr(0x%lx), Val(0x%x)\n"
			LOG_DBG(TMP_STR, module, (unsigned long)regBase,
			(unsigned long)(pReg[i].Addr),
			(unsigned int)(pReg[i].Val));
#undef TMP_STR
		}

		if (((regBase + pReg[i].Addr) < (regBase + PAGE_SIZE))
			&& ((pReg[i].Addr & 0x3) == 0))
			ISP_WR32(regBase + pReg[i].Addr, pReg[i].Val);
		else
			LOG_ERR("wrong address(0x%lx)\n",
				(unsigned long)(regBase + pReg[i].Addr));

	}

	/*  */
	return Ret;
}



/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_WriteReg(struct ISP_REG_IO_STRUCT *pRegIo)
{
	int Ret = 0;
	/*    int TimeVd = 0;*/
	/*    int TimeExpdone = 0;*/
	/*    int TimeTasklet = 0;*/
	/* unsigned char* pData = NULL; */
	struct ISP_REG_STRUCT *pData = NULL;

	if (pRegIo->Count > 0xFFFFFFFF) {
		LOG_ERR("pRegIo->Count error");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	if (IspInfo.DebugMask & ISP_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n",
			(pRegIo->pData), (pRegIo->Count));

	pData = kmalloc((pRegIo->Count) * sizeof(struct ISP_REG_STRUCT),
		GFP_ATOMIC);
	if (pData == NULL) {
		LOG_DBG("ERROR: kmalloc fail, (proc, pid, tgid)=(%s, %d, %d)\n",
		current->comm, current->pid, current->tgid);
		Ret = -ENOMEM;
		goto EXIT;
	}

	if ((void __user *)(pRegIo->pData) == NULL) {
		LOG_ERR("NULL pData\n");
		Ret = -EFAULT;
		goto EXIT;
	}

	/*  */
	if (copy_from_user(pData, (void __user *)(pRegIo->pData),
		pRegIo->Count * sizeof(struct ISP_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = ISP_WriteRegToHw(
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
 ******************************************************************************/
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
static int isp_allocbuf(struct isp_imem_memory *pMemInfo)
{
	int ret = 0;
	struct ion_mm_data mm_data;
	struct ion_sys_data sys_data;
	struct ion_handle *handle = NULL;

	if (pMemInfo == NULL) {
		LOG_ERR("pMemInfo is NULL!!\n");
		ret = -ENOMEM;
		goto isp_allocbuf_exit;
	}

	if (isp_p2_ion_client == NULL) {
		LOG_ERR("isp_p2_ion_client is NULL!!\n");
		ret = -ENOMEM;
		goto isp_allocbuf_exit;
	}

	handle = ion_alloc(isp_p2_ion_client, pMemInfo->length, 0,
		ION_HEAP_MULTIMEDIA_MASK, 0);
	if (handle == NULL) {
		LOG_ERR("fail to alloc ion buffer, ret=%d\n", ret);
		ret = -ENOMEM;
		goto isp_allocbuf_exit;
	}
	pMemInfo->handle = (void *) handle;

	pMemInfo->va = (uintptr_t) ion_map_kernel(isp_p2_ion_client, handle);
	if (pMemInfo->va == 0) {
		LOG_ERR("fail to map va of buffer!\n");
		ret = -ENOMEM;
		goto isp_allocbuf_exit;
	}

	mm_data.mm_cmd = ION_MM_CONFIG_BUFFER;
	mm_data.config_buffer_param.kernel_handle = handle;
	mm_data.config_buffer_param.module_id = 0;
	mm_data.config_buffer_param.security = 0;
	mm_data.config_buffer_param.coherent = 1;
	ret = ion_kernel_ioctl(isp_p2_ion_client, ION_CMD_MULTIMEDIA,
		(unsigned long)&mm_data);
	if (ret) {
		LOG_ERR("fail to config ion buffer, ret=%d\n", ret);
		ret = -ENOMEM;
		goto isp_allocbuf_exit;
	}

	sys_data.sys_cmd = ION_SYS_GET_PHYS;
	sys_data.get_phys_param.kernel_handle = handle;
	ret = ion_kernel_ioctl(isp_p2_ion_client, ION_CMD_SYSTEM,
		(unsigned long)&sys_data);
	pMemInfo->pa = sys_data.get_phys_param.phy_addr;

isp_allocbuf_exit:

	if (ret < 0) {
		if (handle)
			ion_free(isp_p2_ion_client, handle);
	}

	return ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static void isp_freebuf(struct isp_imem_memory *pMemInfo)
{
	struct ion_handle *handle;

	if (pMemInfo == NULL) {
		LOG_ERR("pMemInfo is NULL!!\n");
		return;
	}

	handle = (struct ion_handle *) pMemInfo->handle;
	if (handle != NULL) {
		ion_unmap_kernel(isp_p2_ion_client, handle);
		ion_free(isp_p2_ion_client, handle);
	}

}
#endif

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_DumpBuffer(struct ISP_DUMP_BUFFER_STRUCT *pDumpBufStruct)
{
	int Ret = 0;

	if (pDumpBufStruct->BytesofBufferSize > 0xFFFFFFFF) {
		LOG_ERR("pDumpTuningBufStruct->BytesofBufferSize error\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	if ((void __user *)(pDumpBufStruct->pBuffer) == NULL) {
		LOG_ERR("NULL pDumpBufStruct->pBuffer\n");
		Ret = -EFAULT;
		goto EXIT;
	}
    /* Native Exception */
	switch (pDumpBufStruct->DumpCmd) {
	case ISP_DUMP_TPIPEBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize >
			MAX_ISP_TILE_TDR_HEX_NO) {
			LOG_ERR("tpipe size error\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pTpipeBuffer == NULL)
				g_pTpipeBuffer =
					vmalloc(MAX_ISP_TILE_TDR_HEX_NO);
			else
				LOG_ERR("g_pTpipeBuffer:0x%pK is not NULL!!\n",
					g_pTpipeBuffer);
		}
		if (g_pTpipeBuffer != NULL) {
			if (copy_from_user(g_pTpipeBuffer,
				(void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
				LOG_ERR("copy_from_user g_pTpipeBuffer fail\n");
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
#define ERR_STR "g_pTpipeBuffer kmalloc failed, g_bIonBufAllocated:%d\n"
			LOG_ERR(ERR_STR, g_bIonBufferAllocated);
#undef ERR_STR
		}
#else
		if (copy_from_user(g_TpipeBuffer,
			(void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_TpipeBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p tpipebuf:0x%p is done!!\n",
			pDumpBufStruct->pBuffer, g_pTpipeBuffer);
		DumpBufferField = DumpBufferField | 0x1;
		break;
	case ISP_DUMP_TUNINGBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize > ISP_DIP_REG_SIZE) {
			LOG_ERR("tuning buf size error, size:0x%x\n",
				pDumpBufStruct->BytesofBufferSize);
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pTuningBuffer == NULL)
				g_pTuningBuffer = vmalloc(ISP_DIP_REG_SIZE);
			else
				LOG_ERR("g_TuningBuffer:0x%pK is not NULL!!\n",
					g_pTuningBuffer);
		}
		if (g_pTuningBuffer != NULL) {
			if (copy_from_user(g_pTuningBuffer,
				(void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
#define ERR_STR "copy_from_user g_pTuningBuffer failed\n"
				LOG_ERR(ERR_STR);
#undef ERR_STR
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("ERROR: g_TuningBuffer kmalloc failed\n");
		}
#else
		if (copy_from_user(g_TuningBuffer,
			(void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_TuningBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p tuningbuf:0x%p is done!!\n",
			pDumpBufStruct->pBuffer, g_pTuningBuffer);
		DumpBufferField = DumpBufferField | 0x2;
		break;
	case ISP_DUMP_ISPVIRBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize > ISP_DIP_REG_SIZE) {
			LOG_ERR("vir isp buffer size error, size:0x%x",
				pDumpBufStruct->BytesofBufferSize);
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pVirISPBuffer == NULL)
				g_pVirISPBuffer = vmalloc(ISP_DIP_REG_SIZE);
			else
				LOG_ERR("g_pVirISPBuffer:0x%pK is not NULL!!\n",
					g_pVirISPBuffer);
		}
		if (g_pVirISPBuffer != NULL) {
			if (copy_from_user(g_pVirISPBuffer,
				(void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
#define ERR_STR "copy_from_user g_pVirISPBuffer failed\n"
				LOG_ERR(ERR_STR);
#undef ERR_STR
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("ERROR: g_pVirISPBuffer kmalloc failed\n");
		}
#else
		if (copy_from_user(g_VirISPBuffer,
			(void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_VirISPBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p virispbuf:0x%p is done!!\n",
			pDumpBufStruct->pBuffer, g_pVirISPBuffer);
		DumpBufferField = DumpBufferField | 0x4;
		break;
	case ISP_DUMP_CMDQVIRBUF_CMD:
		if (pDumpBufStruct->BytesofBufferSize >
			MAX_ISP_CMDQ_BUFFER_SIZE) {
			LOG_ERR("cmdq buffer size error, size:0x%x\n",
				pDumpBufStruct->BytesofBufferSize);
			Ret = -EFAULT;
			goto EXIT;
		}
#ifdef AEE_DUMP_REDUCE_MEMORY
		if (g_bIonBufferAllocated == MFALSE) {
			if (g_pCmdqBuffer == NULL)
				g_pCmdqBuffer =
					vmalloc(MAX_ISP_CMDQ_BUFFER_SIZE);
			else
				LOG_ERR("g_pCmdqBuffer:0x%pK is not NULL!!\n",
					g_pCmdqBuffer);
		}
		if (g_pCmdqBuffer != NULL) {
			if (copy_from_user(g_pCmdqBuffer,
				(void __user *)(pDumpBufStruct->pBuffer),
				pDumpBufStruct->BytesofBufferSize) != 0) {
				LOG_ERR("copy_from_user g_pCmdqBuffer fail\n");
				Ret = -EFAULT;
				goto EXIT;
			}
		} else {
			LOG_ERR("ERROR: g_pCmdqBuffer kmalloc failed\n");
		}
#else
		if (copy_from_user(g_CmdqBuffer,
			(void __user *)(pDumpBufStruct->pBuffer),
			pDumpBufStruct->BytesofBufferSize) != 0) {
			LOG_ERR("copy_from_user g_VirISPBuffer failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}
#endif
		LOG_INF("copy dumpbuf::0x%p cmdqbuf:0x%p is done!!\n",
			pDumpBufStruct->pBuffer, g_pCmdqBuffer);
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
			LOG_INF("DumpBufferField:0x%x, g_bUserBufIsReady:%d!\n",
				DumpBufferField, g_bUserBufIsReady);
		}
	}
	/*  */
EXIT:

	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_SetMemInfo(struct ISP_MEM_INFO_STRUCT *pMemInfoStruct)
{
	int Ret = 0;
	/*  */
	if ((void __user *)(pMemInfoStruct->MemVa) == NULL) {
		LOG_ERR("NULL pMemInfoStruct->MemVa");
		Ret = -EFAULT;
		goto EXIT;
	}
	switch (pMemInfoStruct->MemInfoCmd) {
	case ISP_MEMORY_INFO_TPIPE_CMD:
		memcpy(&g_TpipeBaseAddrInfo, pMemInfoStruct,
			sizeof(struct ISP_MEM_INFO_STRUCT));
		LOG_INF("set tpipe memory info is done!!\n");
		break;
	case ISP_MEMORY_INFO_CMDQ_CMD:
		memcpy(&g_CmdqBaseAddrInfo, pMemInfoStruct,
			sizeof(struct ISP_MEM_INFO_STRUCT));
		LOG_INF("set comq memory info is done!!\n");
		break;
	default:
		LOG_ERR("error set memory info cmd:%d",
			pMemInfoStruct->MemInfoCmd);
		break;
	}
	/*  */
EXIT:

	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static atomic_t g_imem_ref_cnt[ISP_REF_CNT_ID_MAX];
/*  */
/* static long ISP_REF_CNT_CTRL_FUNC(unsigned int Param) */
static long ISP_REF_CNT_CTRL_FUNC(unsigned long Param)
{
	int Ret = 0;
	struct ISP_REF_CNT_CTRL_STRUCT ref_cnt_ctrl;
	int imem_ref_cnt = 0;

	/* LOG_INF("[rc]+ QQ"); */ /* for memory corruption check */


	/* //////////////////---add lock here */
	/* spin_lock_irq(&(IspInfo.SpinLock)); */
	/* ////////////////// */
	/*  */
	if (IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL)
		LOG_DBG("[rc]+");

	/*  */
	if (NULL == (void __user *)Param)  {
		LOG_ERR("[rc]NULL Param");
		/* //////////////////---add unlock here */
		/* spin_unlock_irqrestore(&(IspInfo.SpinLock), flags); */
		/* ////////////////// */
		return -EFAULT;
	}
	/*  */
	if (copy_from_user(&ref_cnt_ctrl, (void __user *)Param,
		sizeof(struct ISP_REF_CNT_CTRL_STRUCT)) == 0) {


		if (IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL)
			LOG_DBG("[rc]ctrl(%d),id(%d)", ref_cnt_ctrl.ctrl,
				ref_cnt_ctrl.id);

		/*  */
		if (ref_cnt_ctrl.id < ISP_REF_CNT_ID_MAX) {
			/* //////////////////---add lock here */
			spin_lock(&(IspInfo.SpinLockIspRef));
			/* ////////////////// */
			/*  */
			switch (ref_cnt_ctrl.ctrl) {
			case ISP_REF_CNT_GET:
				break;
			case ISP_REF_CNT_INC:
				atomic_inc(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
				/* g_imem_ref_cnt++; */
				break;
			case ISP_REF_CNT_DEC:
			case ISP_REF_CNT_DEC_AND_RESET_P1_P2_IF_LAST_ONE:
			case ISP_REF_CNT_DEC_AND_RESET_P1_IF_LAST_ONE:
			case ISP_REF_CNT_DEC_AND_RESET_P2_IF_LAST_ONE:
				atomic_dec(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
				/* g_imem_ref_cnt--; */
				break;
			default:
			case ISP_REF_CNT_MAX:
				/* Add this to remove build warning. */
				/* Do nothing. */
				break;
			}
			/*  */
			imem_ref_cnt = (int)atomic_read(
				&g_imem_ref_cnt[ref_cnt_ctrl.id]);

			if (imem_ref_cnt == 0) {
				/* No user left and ctrl is RESET_IF_LAST_ONE,
				 * do ISP reset.
				 */
				if (ref_cnt_ctrl.ctrl ==
				ISP_REF_CNT_DEC_AND_RESET_P1_P2_IF_LAST_ONE ||
					ref_cnt_ctrl.ctrl ==
				ISP_REF_CNT_DEC_AND_RESET_P1_IF_LAST_ONE) {

					ISP_Reset(ISP_REG_SW_CTL_RST_CAM_P1);
					LOG_DBG("Reset P1\n");
				}

				if (ref_cnt_ctrl.ctrl ==
				ISP_REF_CNT_DEC_AND_RESET_P1_P2_IF_LAST_ONE ||
					ref_cnt_ctrl.ctrl ==
				ISP_REF_CNT_DEC_AND_RESET_P2_IF_LAST_ONE)
					ISP_Reset(ISP_REG_SW_CTL_RST_CAM_P2);

			}
			/* //////////////////---add unlock here */
			spin_unlock(&(IspInfo.SpinLockIspRef));
			/* ////////////////// */

			/*  */
			if (IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL)
				LOG_DBG("[rc]ref_cnt(%d)", imem_ref_cnt);

			/*  */
			if ((void __user *)(ref_cnt_ctrl.data_ptr) == NULL) {
				LOG_ERR("NULL data_ptr");
				return -EFAULT;
			}
			if (put_user(imem_ref_cnt,
				(int *)ref_cnt_ctrl.data_ptr) != 0) {
				LOG_ERR("[rc][GET]:copy_to_user failed");
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("[rc]:id(%d) exceed", ref_cnt_ctrl.id);
			Ret = -EFAULT;
		}


	} else {
		LOG_ERR("[rc]copy_from_user failed");
		Ret = -EFAULT;
	}

	/*  */
	if (IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL)
		LOG_DBG("[rc]-");

	/* LOG_INF("[rc]QQ return value:(%d)", Ret); */
	/*  */
	/* //////////////////---add unlock here */
	/* spin_unlock_irqrestore(&(IspInfo.SpinLock), flags); */
	/* ////////////////// */
	return Ret;
}
/*******************************************************************************
 *
 ******************************************************************************/

/*  */
/* isr dbg log , sw isr response counter , +1 when sw receive 1 sof isr. */
static unsigned int sof_count[ISP_IRQ_TYPE_AMOUNT] = {0};
static int Vsync_cnt[2] = {0, 0};

/* keep current frame status */
static enum CAM_FrameST FrameStatus[ISP_IRQ_TYPE_AMOUNT] = {0};

/* current invoked time is at 1st sof or not during each streaming,
 * reset when streaming off
 */
static bool g1stSof[ISP_IRQ_TYPE_AMOUNT] = {0};
#if (TSTMP_SUBSAMPLE_INTPL == 1)
static bool g1stSwP1Done[ISP_IRQ_TYPE_AMOUNT] = {0};
static unsigned long long gPrevSofTimestp[ISP_IRQ_TYPE_AMOUNT];
#endif

static struct S_START_T gSTime[ISP_IRQ_TYPE_AMOUNT] = {{0} };

#ifdef _MAGIC_NUM_ERR_HANDLING_
#define _INVALID_FRM_CNT_ 0xFFFF
#define _MAX_FRM_CNT_ 0xFF

#define _UNCERTAIN_MAGIC_NUM_FLAG_ 0x40000000
#define _DUMMY_MAGIC_              0x20000000
static unsigned int m_LastMNum[_cam_max_] = {0}; /* imgo/rrzo */

#endif
/* static long ISP_Buf_CTRL_FUNC(unsigned int Param) */
static long ISP_Buf_CTRL_FUNC(unsigned long Param)
{
	int Ret = 0;
	enum _isp_dma_enum_ rt_dma;
	unsigned int i = 0;
	/*    unsigned int x = 0;*/
	/*    unsigned int iBuf = 0;*/
	/*    unsigned int size = 0;*/
	/*    unsigned int bWaitBufRdy = 0;*/
	struct ISP_BUFFER_CTRL_STRUCT         rt_buf_ctrl;
	/*    unsigned int flags;*/
	/*    struct ISP_RT_BUF_INFO_STRUCT       rt_buf_info;*/
	/*    struct ISP_DEQUE_BUF_INFO_STRUCT    deque_buf;*/
	/*    enum ISP_IRQ_TYPE_ENUM irqT = ISP_IRQ_TYPE_AMOUNT;*/
	/*    enum ISP_IRQ_TYPE_ENUM irqT_Lock = ISP_IRQ_TYPE_AMOUNT;*/
	/*    bool CurVF_En = MFALSE;*/
	/*  */
	if ((void __user *)Param == NULL)  {
		LOG_ERR("[rtbc]NULL Param");
		return -EFAULT;
	}
	/*  */
	if (copy_from_user(&rt_buf_ctrl, (void __user *)Param,
		sizeof(struct ISP_BUFFER_CTRL_STRUCT)) == 0) {
		if (rt_buf_ctrl.module >= ISP_IRQ_TYPE_AMOUNT) {
			LOG_ERR("[rtbc]not supported module:0x%x\n",
				rt_buf_ctrl.module);
			return -EFAULT;
		}

		if (pstRTBuf[rt_buf_ctrl.module] == NULL)  {
			LOG_ERR("[rtbc]NULL pstRTBuf, module:0x%x\n",
				rt_buf_ctrl.module);
			return -EFAULT;
		}

		rt_dma = rt_buf_ctrl.buf_id;
		if (rt_dma >= _cam_max_) {
			LOG_ERR("[rtbc]buf_id error:0x%x\n", rt_dma);
			return -EFAULT;
		}

		/*  */
		switch (rt_buf_ctrl.ctrl) {
		case ISP_RT_BUF_CTRL_CLEAR:
			/*  */
			if (IspInfo.DebugMask & ISP_DBG_BUF_CTRL)
				LOG_INF("[rtbc][%d][CLEAR]:rt_dma(%d)\n",
					rt_buf_ctrl.module, rt_dma);
			/*  */

			memset((void *)IspInfo.IrqInfo.LastestSigTime_usec[
				rt_buf_ctrl.module],
				0,
				sizeof(unsigned int) * 32);
			memset((void *)IspInfo.IrqInfo.LastestSigTime_sec[
				rt_buf_ctrl.module],
				0,
				sizeof(unsigned int) * 32);

			/* reset active record*/
			pstRTBuf[rt_buf_ctrl.module]->ring_buf[rt_dma].active =
				MFALSE;
			memset((char *)&pstRTBuf[rt_buf_ctrl.module]->ring_buf[
				rt_dma],
				0x00,
				sizeof(struct ISP_RT_RING_BUF_INFO_STRUCT));
			/* init. frmcnt before vf_en */
			for (i = 0; i < ISP_RT_BUF_SIZE; i++)
				pstRTBuf[rt_buf_ctrl.module]->ring_buf[
					rt_dma].data[i].image.frm_cnt =
					_INVALID_FRM_CNT_;

			switch (rt_buf_ctrl.module) {
			case ISP_IRQ_TYPE_INT_CAM_A_ST:
			case ISP_IRQ_TYPE_INT_CAM_B_ST:
				if ((pstRTBuf[rt_buf_ctrl.module]->ring_buf[
					_imgo_].active == MFALSE) &&
					(pstRTBuf[rt_buf_ctrl.module]->ring_buf[
					_rrzo_].active == MFALSE)) {
					sof_count[rt_buf_ctrl.module] = 0;
					g1stSof[rt_buf_ctrl.module] = MTRUE;
					#if (TSTMP_SUBSAMPLE_INTPL == 1)
					g1stSwP1Done[rt_buf_ctrl.module] =
						MTRUE;
					gPrevSofTimestp[rt_buf_ctrl.module] = 0;
					#endif
					g_ISPIntErr[rt_buf_ctrl.module] = 0;
					pstRTBuf[rt_buf_ctrl.module]->dropCnt =
						0;
					pstRTBuf[rt_buf_ctrl.module]->state = 0;
				}

				memset((void *)g_DmaErr_CAM[rt_buf_ctrl.module],
					0, sizeof(unsigned int)*_cam_max_);
				break;
			case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
				if (pstRTBuf[rt_buf_ctrl.module]->ring_buf[
					_camsv_imgo_].active == MFALSE) {
					sof_count[rt_buf_ctrl.module] = 0;
					g1stSof[rt_buf_ctrl.module] = MTRUE;
					g_ISPIntErr[rt_buf_ctrl.module] = 0;
					pstRTBuf[rt_buf_ctrl.module]->dropCnt =
						0;
					pstRTBuf[rt_buf_ctrl.module]->state = 0;
				}

				break;
			case ISP_IRQ_TYPE_INT_DIP_A_ST:
			case ISP_IRQ_TYPE_INT_UNI_A_ST:
				sof_count[rt_buf_ctrl.module] = 0;
				g1stSof[rt_buf_ctrl.module] = MTRUE;
				g_ISPIntErr[rt_buf_ctrl.module] = 0;
				pstRTBuf[rt_buf_ctrl.module]->dropCnt = 0;
				pstRTBuf[rt_buf_ctrl.module]->state = 0;
				break;
			default:
				LOG_ERR("unsupported module:0x%x\n",
					rt_buf_ctrl.module);
				break;
			}

#ifdef _MAGIC_NUM_ERR_HANDLING_
			m_LastMNum[rt_dma] = 0;
#endif

			break;
		case ISP_RT_BUF_CTRL_DMA_EN: {
			unsigned char array[_cam_max_];
			unsigned int z;
			unsigned char *pExt;

			if (rt_buf_ctrl.pExtend == NULL) {
				LOG_ERR("NULL pExtend\n");
				Ret = -EFAULT;
				break;
			}

			pExt = (unsigned char *)(rt_buf_ctrl.pExtend);
			for (z = 0; z < _cam_max_; z++) {
				if (get_user(array[z], (unsigned char *)pExt) ==
					0) {
					pstRTBuf[rt_buf_ctrl.module]->ring_buf[
						z].active = array[z];
					if (IspInfo.DebugMask &
						ISP_DBG_BUF_CTRL) {
#define TMP_STR "[rtbc][DMA_EN]:dma_%d:%d\n"
						LOG_INF(TMP_STR, z, array[z]);
#undef TMP_STR
					}
				} else {
#define ERR_STR "[rtbc][DMA_EN]:get_user failed(%d)\n"
					LOG_ERR(ERR_STR, z);
#undef ERR_STR
					Ret = -EFAULT;
				}
				pExt++;
			}
		}
		break;
		case ISP_RT_BUF_CTRL_MAX:
			/* Add this to remove build warning. */
			/* Do nothing. */
			break;

		}
	} else {
		LOG_ERR("[rtbc]copy_from_user failed");
		Ret = -EFAULT;
	}

	return Ret;
}



/*******************************************************************************
 * update current idnex to working frame
 ******************************************************************************/
static int ISP_P2_BufQue_Update_ListCIdx(enum ISP_P2_BUFQUE_PROPERTY property,
	enum ISP_P2_BUFQUE_LIST_TAG listTag)
{
	int ret = 0;
	int tmpIdx = 0;
	int cnt = 0;
	bool stop = false;
	int i = 0;
	enum ISP_P2_BUF_STATE_ENUM cIdxSts = ISP_P2_BUF_STATE_NONE;

	switch (listTag) {
	case ISP_P2_BUFQUE_LIST_TAG_UNIT:
		/* [1] check global pointer current sts */
		cIdxSts = P2_FrameUnit_List[property][
			P2_FrameUnit_List_Idx[property].curr].bufSts;
		/* ////////////////////////////////////////////////////////// */
		/* Assume we have the buffer list in the following situation */
		/* ++++++         ++++++         ++++++ */
		/* +  vss +         +  prv +         +  prv + */
		/* ++++++         ++++++         ++++++ */
		/* not deque         erased           enqued */
		/* done */
		/*  */
		/* if the vss deque is done, we should update the CurBufIdx to
		 * the next "enqued" buffer node instead of just moving to the
		 * next buffer node
		 */
		/* ////////////////////////////////////////////////////////// */
		/* [2]traverse count needed */
		if (P2_FrameUnit_List_Idx[property].start <=
			P2_FrameUnit_List_Idx[property].end) {
			cnt = P2_FrameUnit_List_Idx[property].end -
				P2_FrameUnit_List_Idx[property].start;
		} else {
			cnt = _MAX_SUPPORT_P2_FRAME_NUM_ -
				P2_FrameUnit_List_Idx[property].start;
			cnt += P2_FrameUnit_List_Idx[property].end;
		}

		/* [3] update current index for frame unit list */
		tmpIdx = P2_FrameUnit_List_Idx[property].curr;
		switch (cIdxSts) {
		case ISP_P2_BUF_STATE_ENQUE:
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[
				property].curr].bufSts =
					ISP_P2_BUF_STATE_RUNNING;
			break;
		case ISP_P2_BUF_STATE_DEQUE_SUCCESS:
			do { /* to find the newest cur index */
				tmpIdx = (tmpIdx + 1) %
					_MAX_SUPPORT_P2_FRAME_NUM_;
				switch (P2_FrameUnit_List[property][
					tmpIdx].bufSts) {
				case ISP_P2_BUF_STATE_ENQUE:
				case ISP_P2_BUF_STATE_RUNNING:
					P2_FrameUnit_List[property][
						tmpIdx].bufSts =
						ISP_P2_BUF_STATE_RUNNING;
					P2_FrameUnit_List_Idx[property].curr =
						tmpIdx;
					stop = true;
					break;
				case ISP_P2_BUF_STATE_WAIT_DEQUE_FAIL:
				case ISP_P2_BUF_STATE_DEQUE_SUCCESS:
				case ISP_P2_BUF_STATE_DEQUE_FAIL:
				case ISP_P2_BUF_STATE_NONE:
				default:
					break;
				}
				i++;
			} while ((i < cnt) && (!stop));
			/* ////////////////////////////////////////////////// */
			/* Assume we have the buffer list in the following
			 * situation:
			 */
			/* ++++++         ++++++         ++++++ */
			/* +  vss +         +  prv +         +  prv + */
			/* ++++++         ++++++         ++++++ */
			/* not deque         erased           erased */
			/* done */
			/*  */
			/* all the buffer node are deque done in the current
			 * moment, should update current index to the last node.
			 */
			/* if the vss deque is done, we should update the
			 * CurBufIdx to the last buffer node
			 */
			/* ////////////////////////////////////////////////// */
			if ((!stop) && (i == (cnt)))
				P2_FrameUnit_List_Idx[property].curr =
					P2_FrameUnit_List_Idx[property].end;

			break;
		case ISP_P2_BUF_STATE_WAIT_DEQUE_FAIL:
		case ISP_P2_BUF_STATE_DEQUE_FAIL:
			/* QQ. ADD ASSERT */
			break;
		case ISP_P2_BUF_STATE_NONE:
		case ISP_P2_BUF_STATE_RUNNING:
		default:
			break;
		}
		break;
	case ISP_P2_BUFQUE_LIST_TAG_PACKAGE:
	default:
		LOG_ERR("Wrong List tag(%d)\n", listTag);
		break;
	}
	return ret;
}
/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_P2_BufQue_Erase(enum ISP_P2_BUFQUE_PROPERTY property,
	enum ISP_P2_BUFQUE_LIST_TAG listTag, int idx)
{
	int ret =  -1;
	bool stop = false;
	int i = 0;
	int cnt = 0;
	int tmpIdx = 0;

	switch (listTag) {
	case ISP_P2_BUFQUE_LIST_TAG_PACKAGE:
		tmpIdx = P2_FramePack_List_Idx[property].start;
		/* [1] clear buffer status */
		P2_FramePackage_List[property][idx].processID = 0x0;
		P2_FramePackage_List[property][idx].callerID = 0x0;
		P2_FramePackage_List[property][idx].dupCQIdx =  -1;
		P2_FramePackage_List[property][idx].frameNum = 0;
		P2_FramePackage_List[property][idx].dequedNum = 0;
		/* [2] update first index */
		if (P2_FramePackage_List[property][tmpIdx].dupCQIdx == -1) {
			/* traverse count needed, cuz user may erase the
			 * element but not the one at first idx(pip or vss
			 * scenario)
			 */
			if (P2_FramePack_List_Idx[property].start <=
				P2_FramePack_List_Idx[property].end) {
				cnt = P2_FramePack_List_Idx[property].end -
					P2_FramePack_List_Idx[property].start;
			} else {
				cnt = _MAX_SUPPORT_P2_PACKAGE_NUM_ -
					P2_FramePack_List_Idx[property].start;
				cnt += P2_FramePack_List_Idx[property].end;
			}
			do { /* to find the newest first lindex */
				tmpIdx = (tmpIdx + 1) %
					_MAX_SUPPORT_P2_PACKAGE_NUM_;

				switch (P2_FramePackage_List[property][
					tmpIdx].dupCQIdx) {
				case (-1):
					break;
				default:
					stop = true;
					P2_FramePack_List_Idx[property].start =
						tmpIdx;
					break;
				}
				i++;
			} while ((i < cnt) && (!stop));
			/* current last erased element in list is the one
			 * firstBufindex point at and all the buffer node are
			 * deque done in the current moment, should update first
			 * index to the last node.
			 */
			if ((!stop) && (i == cnt))
				P2_FramePack_List_Idx[property].start =
					P2_FramePack_List_Idx[property].end;

		}
		break;
	case ISP_P2_BUFQUE_LIST_TAG_UNIT:
		tmpIdx = P2_FrameUnit_List_Idx[property].start;
		/* [1] clear buffer status */
		P2_FrameUnit_List[property][idx].processID = 0x0;
		P2_FrameUnit_List[property][idx].callerID = 0x0;
		P2_FrameUnit_List[property][idx].cqMask =  0x0;
		P2_FrameUnit_List[property][idx].bufSts = ISP_P2_BUF_STATE_NONE;
		/* [2]update first index */
		if (P2_FrameUnit_List[property][tmpIdx].bufSts ==
			ISP_P2_BUF_STATE_NONE) {
			/* traverse count needed, cuz user may erase the element
			 * but not the one at first idx
			 */
			if (P2_FrameUnit_List_Idx[property].start <=
				P2_FrameUnit_List_Idx[property].end) {
				cnt = P2_FrameUnit_List_Idx[property].end -
					P2_FrameUnit_List_Idx[property].start;
			} else {
				cnt = _MAX_SUPPORT_P2_FRAME_NUM_ -
					P2_FrameUnit_List_Idx[property].start;
				cnt += P2_FrameUnit_List_Idx[property].end;
			}
			/* to find the newest first lindex */
			do {
				tmpIdx = (tmpIdx + 1) %
					_MAX_SUPPORT_P2_FRAME_NUM_;
				switch (P2_FrameUnit_List[property][
					tmpIdx].bufSts) {
				case ISP_P2_BUF_STATE_ENQUE:
				case ISP_P2_BUF_STATE_RUNNING:
				case ISP_P2_BUF_STATE_DEQUE_SUCCESS:
					stop = true;
					P2_FrameUnit_List_Idx[property].start =
						tmpIdx;
					break;
				case ISP_P2_BUF_STATE_WAIT_DEQUE_FAIL:
				case ISP_P2_BUF_STATE_DEQUE_FAIL:
					/* ASSERT */
					break;
				case ISP_P2_BUF_STATE_NONE:
				default:
					break;
				}
				i++;
			} while ((i < cnt) && (!stop));
			/* current last erased element in list is the one
			 * firstBufindex point at and all the buffer node are
			 * deque done in the current moment, should update
			 * first index to the last node
			 */
			if ((!stop) && (i == (cnt)))
				P2_FrameUnit_List_Idx[property].start =
					P2_FrameUnit_List_Idx[property].end;

		}
		break;
	default:
		break;
	}
	return ret;
}

/*******************************************************************************
 * get first matched element idnex
 ******************************************************************************/
static int ISP_P2_BufQue_GetMatchIdx(struct ISP_P2_BUFQUE_STRUCT param,
	enum ISP_P2_BUFQUE_MATCH_TYPE matchType,
	enum ISP_P2_BUFQUE_LIST_TAG listTag)
{
	int idx = -1;
	int i = 0;
	int property;

	if (param.property >= ISP_P2_BUFQUE_PROPERTY_NUM) {
		LOG_ERR("property err(%d)\n", param.property);
		return idx;
	}
	property = param.property;

	switch (matchType) {
	case ISP_P2_BUFQUE_MATCH_TYPE_WAITDQ:
		/* traverse for finding the frame unit which had not beed
		 * dequeued of the process.
		 */
		if (P2_FrameUnit_List_Idx[property].start <=
			P2_FrameUnit_List_Idx[property].end) {
			for (i = P2_FrameUnit_List_Idx[property].start;
				i <= P2_FrameUnit_List_Idx[property].end; i++) {
				if ((P2_FrameUnit_List[property][i].processID ==
					param.processID) && ((P2_FrameUnit_List[
						property][i].bufSts ==
					ISP_P2_BUF_STATE_ENQUE) ||
					(P2_FrameUnit_List[property][
						i].bufSts ==
					ISP_P2_BUF_STATE_RUNNING))) {
					idx = i;
					break;
				}
			}
		} else {
			for (i = P2_FrameUnit_List_Idx[property].start;
				i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
				if ((P2_FrameUnit_List[property][i].processID ==
					param.processID) && ((P2_FrameUnit_List[
						property][i].bufSts ==
					ISP_P2_BUF_STATE_ENQUE) ||
					(P2_FrameUnit_List[property][
						i].bufSts ==
					ISP_P2_BUF_STATE_RUNNING))) {
					idx = i;
					break;
				}
			}
			if (idx !=  -1) {
				/*get in the first for loop*/
			} else {
				for (i = 0; i <=
					P2_FrameUnit_List_Idx[property].end;
					i++) {
					if ((P2_FrameUnit_List[property][
						i].processID ==
						param.processID) &&
						((P2_FrameUnit_List[property][
							i].bufSts ==
						ISP_P2_BUF_STATE_ENQUE) ||
						(P2_FrameUnit_List[property][
							i].bufSts ==
						ISP_P2_BUF_STATE_RUNNING))) {
						idx = i;
						break;
					}
				}
			}
		}
		break;
	case ISP_P2_BUFQUE_MATCH_TYPE_WAITFM:
		if (P2_FramePack_List_Idx[property].start <=
			P2_FramePack_List_Idx[property].end) {
			for (i = P2_FramePack_List_Idx[property].start; i <=
				P2_FramePack_List_Idx[property].end; i++) {
				if ((P2_FramePackage_List[property][
					i].processID == param.processID) &&
					(P2_FramePackage_List[property][
					i].callerID == param.callerID)) {
					idx = i;
					break;
				}
			}
		} else {
			for (i = P2_FramePack_List_Idx[property].start;
				i < _MAX_SUPPORT_P2_PACKAGE_NUM_; i++) {
				if ((P2_FramePackage_List[property][
					i].processID == param.processID) &&
					(P2_FramePackage_List[property][
					i].callerID == param.callerID)) {
					idx = i;
					break;
				}
			}
			if (idx !=  -1) {
				/*get in the first for loop*/
			} else {
				for (i = 0; i <=
					P2_FramePack_List_Idx[property].end;
					i++) {
					if ((P2_FramePackage_List[property][
						i].processID ==
						param.processID) &&
						(P2_FramePackage_List[property][
							i].callerID ==
						param.callerID)) {
						idx = i;
						break;
					}
				}
			}
		}
		break;
	case ISP_P2_BUFQUE_MATCH_TYPE_FRAMEOP:
		/* deque done notify */
		if (listTag == ISP_P2_BUFQUE_LIST_TAG_PACKAGE) {
			if (P2_FramePack_List_Idx[property].start <=
				P2_FramePack_List_Idx[property].end) {
				for (i = P2_FramePack_List_Idx[property].start;
					i <= P2_FramePack_List_Idx[
					property].end; i++) {
					if ((P2_FramePackage_List[property][
						i].processID ==
						param.processID) &&
						(P2_FramePackage_List[property][
							i].callerID ==
						param.callerID) &&
						(P2_FramePackage_List[property][
							i].dupCQIdx ==
						param.dupCQIdx) &&
						(P2_FramePackage_List[property][
							i].dequedNum <
						P2_FramePackage_List[property][
							i].frameNum)) {
						/* avoid race that dupCQ_1 of
						 * buffer2 enqued while dupCQ_1
						 * of buffer1 have beend deque
						 * done but not been erased yet.
						 */
						idx = i;
						break;
					}
				}
			} else {
				for (i = P2_FramePack_List_Idx[property].start;
					i < _MAX_SUPPORT_P2_PACKAGE_NUM_; i++) {
					if ((P2_FramePackage_List[property][
						i].processID ==
						param.processID) &&
						(P2_FramePackage_List[property][
						i].callerID ==
						param.callerID) &&
						(P2_FramePackage_List[property][
						i].dupCQIdx ==
						param.dupCQIdx) &&
						(P2_FramePackage_List[property][
						i].dequedNum <
						P2_FramePackage_List[property][
						i].frameNum)) {
						idx = i;
						break;
					}
				}
				if (idx !=  -1) {
					/*get in the first for loop*/
					break;
				}
				for (i = 0; i <=
					P2_FramePack_List_Idx[property].end;
					i++) {
					if ((P2_FramePackage_List[property][
						i].processID ==
						param.processID) &&
						(P2_FramePackage_List[property][
						i].callerID ==
						param.callerID) &&
						(P2_FramePackage_List[property][
						i].dupCQIdx ==
						param.dupCQIdx) &&
						(P2_FramePackage_List[property][
						i].dequedNum <
						P2_FramePackage_List[property][
						i].frameNum)) {
						idx = i;
						break;
					}
				}
			}
		} else {
			if (P2_FrameUnit_List_Idx[property].start <=
				P2_FrameUnit_List_Idx[property].end) {
				for (i = P2_FrameUnit_List_Idx[property].start;
					i <= P2_FrameUnit_List_Idx[
					property].end; i++) {
					if ((P2_FrameUnit_List[property][
						i].processID ==
						param.processID) &&
						(P2_FrameUnit_List[property][
						i].callerID ==
						param.callerID)) {
						idx = i;
						break;
					}
				}
			} else {
				for (i = P2_FrameUnit_List_Idx[property].start;
					i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
					if ((P2_FrameUnit_List[property][
						i].processID ==
						param.processID) &&
						(P2_FrameUnit_List[property][
						i].callerID ==
						param.callerID)) {
						idx = i;
						break;
					}
				}
				if (idx !=  -1) {
					/*get in the first for loop*/
					break;
				}
				for (i = 0; i <=
					P2_FrameUnit_List_Idx[property].end;
					i++) {
					if ((P2_FrameUnit_List[property][
						i].processID ==
						param.processID) &&
						(P2_FrameUnit_List[property][
						i].callerID ==
						param.callerID)) {
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
 ******************************************************************************/
static inline unsigned int ISP_P2_BufQue_WaitEventState(
	struct ISP_P2_BUFQUE_STRUCT param,
	enum ISP_P2_BUFQUE_MATCH_TYPE type, int *idx)
{
	unsigned int ret = MFALSE;
	int index = -1;
	enum ISP_P2_BUFQUE_PROPERTY property;

	if (param.property >= ISP_P2_BUFQUE_PROPERTY_NUM) {
		LOG_ERR("property err(%d)\n", param.property);
		return ret;
	}
	property = param.property;
	/*  */
	switch (type) {
	case ISP_P2_BUFQUE_MATCH_TYPE_WAITDQ:
		spin_lock(&(SpinLock_P2FrameList));
		index = *idx;
		if (P2_FrameUnit_List[property][index].bufSts ==
			ISP_P2_BUF_STATE_RUNNING)
			ret = MTRUE;

		spin_unlock(&(SpinLock_P2FrameList));
		break;
	case ISP_P2_BUFQUE_MATCH_TYPE_WAITFM:
		spin_lock(&(SpinLock_P2FrameList));
		index = *idx;
		if (P2_FramePackage_List[property][index].dequedNum ==
			P2_FramePackage_List[property][index].frameNum)
			ret = MTRUE;

		spin_unlock(&(SpinLock_P2FrameList));
		break;
	case ISP_P2_BUFQUE_MATCH_TYPE_WAITFMEQD:
		spin_lock(&(SpinLock_P2FrameList));
		index = ISP_P2_BufQue_GetMatchIdx(param,
			ISP_P2_BUFQUE_MATCH_TYPE_WAITFM,
			ISP_P2_BUFQUE_LIST_TAG_PACKAGE);
		if (index == -1) {
			spin_unlock(&(SpinLock_P2FrameList));
			ret = MFALSE;
		} else {
			*idx = index;
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
 ******************************************************************************/
static int ISP_P2_BufQue_CTRL_FUNC(struct ISP_P2_BUFQUE_STRUCT param)
{
	int ret = 0;
	int i = 0, q = 0;
	int idx =  -1, idx2 =  -1;
	int restTime = 0;
	int property;
	unsigned int p2_event = 0;

	if (param.property >= ISP_P2_BUFQUE_PROPERTY_NUM) {
		LOG_ERR("property err(%d)\n", param.property);
		ret = -EFAULT;
		return ret;
	}
	property = param.property;

	switch (param.ctrl) {
	case ISP_P2_BUFQUE_CTRL_ENQUE_FRAME:
	{
		unsigned int cqmask = (param.dupCQIdx << 2) |
			(param.cQIdx << 1) | (param.burstQIdx);
		/* signal that a specific buffer is enqueued */
		spin_lock(&(SpinLock_P2FrameList));
		/* (1) check the ring buffer list is full or not */
		if (((P2_FramePack_List_Idx[property].end + 1) %
			_MAX_SUPPORT_P2_PACKAGE_NUM_) ==
			P2_FramePack_List_Idx[property].start &&
			(P2_FramePack_List_Idx[property].end !=  -1)) {
#define ERR_STR1 "pty(%d), F/L(%d_%d,%d), dCQ(%d,%d),"
#define ERR_STR2 " RF/C/L(%d,%d,%d), sts(%d,%d,%d)\n"
			LOG_ERR(ERR_STR1 ERR_STR2,
				property, param.frameNum,
				P2_FramePack_List_Idx[property].start,
				P2_FramePack_List_Idx[property].end,
				P2_FramePackage_List[property][
					P2_FramePack_List_Idx[
					property].start].dupCQIdx,
				P2_FramePackage_List[property][
					P2_FramePack_List_Idx[
					property].end].dupCQIdx,
				P2_FrameUnit_List_Idx[property].start,
				P2_FrameUnit_List_Idx[property].curr,
				P2_FrameUnit_List_Idx[property].end,
				P2_FrameUnit_List[property][
					P2_FrameUnit_List_Idx[
					property].start].bufSts,
				P2_FrameUnit_List[property][
					P2_FrameUnit_List_Idx[
					property].curr].bufSts,
				P2_FrameUnit_List[property][
					P2_FrameUnit_List_Idx[
					property].end].bufSts);
#undef ERR_STR1
#undef ERR_STR2
			spin_unlock(&(SpinLock_P2FrameList));
			LOG_ERR("p2 frame package list is full, enque Fail.\n");
			ret =  -EFAULT;
			return ret;
		}

		/*(2) add new to the last of the frame unit list */
		if (P2_FramePack_List_Idx[property].end < 0 ||
			P2_FrameUnit_List_Idx[property].end < 0) {
#ifdef P2_DBG_LOG
#define TMP_STR1 "pty(%d) pD(0x%x_0x%x) MF/L(%d_%d %d) (%d %d)"
#define TMP_STR2 " RF/C/L(%d %d %d) (%d %d %d) cqmsk(0x%x)\n"
			IRQ_LOG_KEEPER(ISP_IRQ_TYPE_INT_DIP_A_ST, 0, _LOG_DBG,
				TMP_STR1 TMP_STR2,
				property, param.processID, param.callerID,
				param.frameNum,
				P2_FramePack_List_Idx[property].start,
				P2_FramePack_List_Idx[property].end,
				P2_FramePackage_List[
					property][P2_FramePack_List_Idx[
					property].start].dupCQIdx,
				P2_FramePackage_List[property][0].dupCQIdx,
				P2_FrameUnit_List_Idx[property].start,
				P2_FrameUnit_List_Idx[property].curr,
				P2_FrameUnit_List_Idx[property].end,
				P2_FrameUnit_List[
					property][P2_FrameUnit_List_Idx[
					property].start].bufSts,
				P2_FrameUnit_List[
					property][P2_FrameUnit_List_Idx[
					property].curr].bufSts,
				P2_FrameUnit_List[property][0].bufSts, cqmask);
#undef TMP_STR1
#undef TMP_STR2
#endif
		} else {
#ifdef P2_DBG_LOG
#define TMP_STR1 "pty(%d) pD(0x%x_0x%x) MF/L(%d_%d %d) (%d %d)"
#define TMP_STR2 " RF/C/L(%d %d %d) (%d %d %d) cqmsk(0x%x)\n"
			IRQ_LOG_KEEPER(ISP_IRQ_TYPE_INT_DIP_A_ST, 0, _LOG_DBG,
				TMP_STR1 TMP_STR2,
				property, param.processID, param.callerID,
				param.frameNum,
				P2_FramePack_List_Idx[property].start,
				P2_FramePack_List_Idx[property].end,
				P2_FramePackage_List[
					property][P2_FramePack_List_Idx[
					property].start].dupCQIdx,
				P2_FramePackage_List[
					property][P2_FramePack_List_Idx[
					property].end].dupCQIdx,
				P2_FrameUnit_List_Idx[property].start,
				P2_FrameUnit_List_Idx[property].curr,
				P2_FrameUnit_List_Idx[property].end,
				P2_FrameUnit_List[
					property][P2_FrameUnit_List_Idx[
					property].start].bufSts,
				P2_FrameUnit_List[
					property][P2_FrameUnit_List_Idx[
					property].curr].bufSts,
				P2_FrameUnit_List[
					property][P2_FrameUnit_List_Idx[
					property].end].bufSts, cqmask);
#undef TMP_STR1
#undef TMP_STR2
#endif
		}
		if (P2_FrameUnit_List_Idx[property].start ==
			P2_FrameUnit_List_Idx[property].end &&
			P2_FrameUnit_List[property][
			P2_FrameUnit_List_Idx[property].start].bufSts ==
			ISP_P2_BUF_STATE_NONE) {
			/* frame unit list is empty */
			P2_FrameUnit_List_Idx[property].end =
				(P2_FrameUnit_List_Idx[property].end + 1) %
				_MAX_SUPPORT_P2_FRAME_NUM_;
			P2_FrameUnit_List_Idx[property].start =
				P2_FrameUnit_List_Idx[property].end;
			P2_FrameUnit_List_Idx[property].curr =
				P2_FrameUnit_List_Idx[property].end;
		} else if (P2_FrameUnit_List_Idx[property].curr ==
			P2_FrameUnit_List_Idx[property].end &&
			P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[
			property].curr].bufSts ==
			ISP_P2_BUF_STATE_NONE) {
			/* frame unit list is not empty, but current/last is
			 * empty. (all the enqueued frame is done but user
			 * have not called dequeue)
			 */
			P2_FrameUnit_List_Idx[property].end =
				(P2_FrameUnit_List_Idx[property].end + 1) %
				_MAX_SUPPORT_P2_FRAME_NUM_;
			P2_FrameUnit_List_Idx[property].curr =
				P2_FrameUnit_List_Idx[property].end;
		} else {
			P2_FrameUnit_List_Idx[property].end =
				(P2_FrameUnit_List_Idx[property].end + 1) %
				_MAX_SUPPORT_P2_FRAME_NUM_;
		}
		P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[
			property].end].processID = param.processID;
		P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[
			property].end].callerID = param.callerID;
		P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[
			property].end].cqMask = cqmask;
		P2_FrameUnit_List[property][P2_FrameUnit_List_Idx[
			property].end].bufSts = ISP_P2_BUF_STATE_ENQUE;

		/* [3] add new frame package in list */
		if (param.burstQIdx == 0) {
			if (P2_FramePack_List_Idx[property].start ==
				P2_FramePack_List_Idx[property].end &&
				P2_FramePackage_List[
					property][P2_FramePack_List_Idx[
					property].start].dupCQIdx == -1) {
				/* all managed buffer node is empty */
				P2_FramePack_List_Idx[property].end =
				(P2_FramePack_List_Idx[property].end + 1) %
					_MAX_SUPPORT_P2_PACKAGE_NUM_;
				P2_FramePack_List_Idx[property].start =
					P2_FramePack_List_Idx[property].end;
			} else {
				P2_FramePack_List_Idx[property].end =
					(P2_FramePack_List_Idx[
					property].end + 1) %
					_MAX_SUPPORT_P2_PACKAGE_NUM_;
			}
			P2_FramePackage_List[property][
				P2_FramePack_List_Idx[property].end].processID =
				param.processID;
			P2_FramePackage_List[property][
				P2_FramePack_List_Idx[property].end].callerID =
				param.callerID;
			P2_FramePackage_List[property][
				P2_FramePack_List_Idx[property].end].dupCQIdx =
				param.dupCQIdx;
			P2_FramePackage_List[property][
				P2_FramePack_List_Idx[property].end].frameNum =
				param.frameNum;
			P2_FramePackage_List[property][
				P2_FramePack_List_Idx[property].end].dequedNum =
				0;
		}

		/* [4]update global index */
		ISP_P2_BufQue_Update_ListCIdx(property,
			ISP_P2_BUFQUE_LIST_TAG_UNIT);
		spin_unlock(&(SpinLock_P2FrameList));
		IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_DIP_A_ST, 0, _LOG_DBG);
		/* [5] wake up thread that wait for deque */
		wake_up_interruptible_all(&P2WaitQueueHead_WaitDeque);
		wake_up_interruptible_all(&P2WaitQueueHead_WaitFrameEQDforDQ);
		break;
		}
	case ISP_P2_BUFQUE_CTRL_WAIT_DEQUE:
		/* a dequeue thread is waiting to do dequeue */
		spin_lock(&(SpinLock_P2FrameList));
		idx = ISP_P2_BufQue_GetMatchIdx(param,
			ISP_P2_BUFQUE_MATCH_TYPE_WAITDQ,
			ISP_P2_BUFQUE_LIST_TAG_UNIT);
		spin_unlock(&(SpinLock_P2FrameList));
		if (idx ==  -1) {
#define ERR_STR1 "Do not find match buffer (pty/pid/cid:"
#define ERR_STR2 " %d/0x%x/0x%x) to deque!\n"
			LOG_ERR(ERR_STR1 ERR_STR2,
				param.property, param.processID,
				param.callerID);
#undef ERR_STR1
#undef ERR_STR2
			ret =  -EFAULT;
			return ret;
		}
		{
			p2_event = ISP_P2_BufQue_WaitEventState(param,
				ISP_P2_BUFQUE_MATCH_TYPE_WAITDQ, &idx);
			if (!p2_event) {
				restTime = wait_event_interruptible_timeout(
				P2WaitQueueHead_WaitDeque,
				p2_event,
				ISP_UsToJiffies(15 * 1000000)); /* 15s */

				if (restTime == 0) {
#define ERR_STR "Wait Deque fail, idx(%d), pty(%d), pID(0x%x),cID(0x%x)\n"
					LOG_ERR(ERR_STR,
						idx, param.property,
						param.processID,
						param.callerID);
#undef ERR_STR
					ret =  -EFAULT;
				} else if (restTime == -512) {
					LOG_ERR("be stopped, restime(%d)\n",
						restTime);
					ret =  -EFAULT;
					break;
				}
			}
		}
		break;
	case ISP_P2_BUFQUE_CTRL_DEQUE_SUCCESS:
		/* signal that a buffer is dequeued(success) */
		if (IspInfo.DebugMask & ISP_DBG_BUF_CTRL)
			LOG_DBG("dq cm(%d),pID(0x%x),cID(0x%x)\n", param.ctrl,
				param.processID, param.callerID);

		spin_lock(&(SpinLock_P2FrameList));
		/* [1]update buffer status for the current buffer */
		/* ////////////////////////////////////////////////////////// */
		/* Assume we have the buffer list in the following situation */
		/* ++++++    ++++++ */
		/* +  vss +    +  prv + */
		/* ++++++    ++++++ */
		/*  */
		/* if the vss deque is not done(not blocking deque), dequeThread
		 * in userspace would change to deque prv buffer(block deque)
		 * immediately to decrease ioctrl count.
		 */
		/* -> vss buffer would be deque at next turn, so curBuf is still
		 * at vss buffer node
		 */
		/* -> we should use param to find the current buffer index in
		 * Rlikst to update the buffer status, cuz deque success/fail
		 * may not be the first buffer in Rlist.
		 */
		/* ////////////////////////////////////////////////////////// */
		idx2 = ISP_P2_BufQue_GetMatchIdx(param,
			ISP_P2_BUFQUE_MATCH_TYPE_FRAMEOP,
			ISP_P2_BUFQUE_LIST_TAG_UNIT);
		if (idx2 ==  -1) {
			spin_unlock(&(SpinLock_P2FrameList));
#define ERR_STR "ERROR findmatch index 2 fail (%d_0x%x_0x%x_%d, %d_%d)\n"
			LOG_ERR(ERR_STR,
				param.property, param.processID, param.callerID,
				param.frameNum, param.cQIdx, param.dupCQIdx);
#undef ERR_STR
			ret =  -EFAULT;
			return ret;
		}
		if (param.ctrl == ISP_P2_BUFQUE_CTRL_DEQUE_SUCCESS)
			P2_FrameUnit_List[property][idx2].bufSts =
				ISP_P2_BUF_STATE_DEQUE_SUCCESS;
		else
			P2_FrameUnit_List[property][idx2].bufSts =
				ISP_P2_BUF_STATE_DEQUE_FAIL;

		/* [2]update dequeued num in managed buffer list */
		idx = ISP_P2_BufQue_GetMatchIdx(param,
			ISP_P2_BUFQUE_MATCH_TYPE_FRAMEOP,
			ISP_P2_BUFQUE_LIST_TAG_PACKAGE);
		if (idx ==  -1) {
			spin_unlock(&(SpinLock_P2FrameList));
#define ERR_STR "ERROR findmatch index 1 fail (%d_0x%x_0x%x_%d, %d_%d)\n"
			LOG_ERR(ERR_STR,
				param.property, param.processID, param.callerID,
				param.frameNum, param.cQIdx, param.dupCQIdx);
#undef ERR_STR
			ret =  -EFAULT;
			return ret;
		}
		P2_FramePackage_List[property][idx].dequedNum++;
		/* [3]update global pointer */
		ISP_P2_BufQue_Update_ListCIdx(property,
			ISP_P2_BUFQUE_LIST_TAG_UNIT);
		/* [4]erase node in ring buffer list */
		ISP_P2_BufQue_Erase(property,
			ISP_P2_BUFQUE_LIST_TAG_UNIT, idx2);
		spin_unlock(&(SpinLock_P2FrameList));
		/* [5]wake up thread user that wait for a specific buffer and
		 * the thread that wait for deque.
		 */
		wake_up_interruptible_all(&P2WaitQueueHead_WaitFrame);
		wake_up_interruptible_all(&P2WaitQueueHead_WaitDeque);
		break;
	case ISP_P2_BUFQUE_CTRL_DEQUE_FAIL:
		/* signal that a buffer is dequeued(fail) */
		/* QQ. ASSERT*/
		break;
	case ISP_P2_BUFQUE_CTRL_WAIT_FRAME:
		/* wait for a specific buffer */
		/* [1]find first match buffer */
		/* wait for frame enqued due to user might call deque api before
		 * the frame is enqued to kernel.
		 */

		/* wait 15s to get paired frame */
		p2_event = ISP_P2_BufQue_WaitEventState(param,
			ISP_P2_BUFQUE_MATCH_TYPE_WAITFMEQD, &idx);
		if (!p2_event) {
			restTime = wait_event_interruptible_timeout(
				P2WaitQueueHead_WaitFrameEQDforDQ,
				p2_event,
				ISP_UsToJiffies(15 * 1000000));

			if (restTime == 0) {
#define ERR_STR1 "could not find match buffer restTime(%d),"
#define ERR_STR2 " pty/pID/cID (%d/0x%x/0x%x), idx(%d)\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					restTime,
					property,
					param.processID,
					param.callerID, idx);
#undef ERR_STR1
#undef ERR_STR2
				ret =  -EFAULT;
				return ret;
			} else if (restTime == -512) {
				LOG_ERR("be stopped, restime(%d)\n", restTime);
				ret =  -EFAULT;
				return ret;
			}
		}
#ifdef P2_DBG_LOG
#define TMP_STR1 "ISP_P2_BUFQUE_CTRL_WAIT_FRAME, after pty/pID/cID,"
#define TMP_STR2 " (%d/0x%x/0x%x), idx(%d)\n"
		LOG_INF(TMP_STR1 TMP_STR2,
			property, param.processID, param.callerID, idx);
#endif
		if (idx ==  -1) {
			LOG_ERR("index is a negative value!");
			ret =  -EFAULT;
			return ret;
		}
		spin_lock(&(SpinLock_P2FrameList));
		/* [2]check the buffer is dequeued or not */
		if (P2_FramePackage_List[property][idx].dequedNum ==
			P2_FramePackage_List[property][idx].frameNum) {
			ISP_P2_BufQue_Erase(property,
				ISP_P2_BUFQUE_LIST_TAG_PACKAGE, idx);
			spin_unlock(&(SpinLock_P2FrameList));
			ret = 0;
#ifdef P2_DBG_LOG
#define TMP_STR1 "Frame is alreay dequeued, return user,"
#define TMP_STR2 " pd(%d/0x%x/0x%x),idx(%d)\n"
			LOG_DBG(TMP_STR1 TMP_STR2,
				property, param.processID, param.callerID, idx);
#undef TMP_STR1
#undef TMP_STR2
#endif
			return ret;
		}

		spin_unlock(&(SpinLock_P2FrameList));
		if (IspInfo.DebugMask & ISP_DBG_BUF_CTRL)
			LOG_DBG("=pd(%d/0x%x/0x%x_%d)wait(%d s)=\n", property,
				param.processID, param.callerID, idx,
				param.timeoutIns);

		/* [3]if not, goto wait event and wait for a signal to check */
		p2_event = ISP_P2_BufQue_WaitEventState(param,
			ISP_P2_BUFQUE_MATCH_TYPE_WAITFM, &idx);
		if (!p2_event) {
			restTime = wait_event_interruptible_timeout(
				P2WaitQueueHead_WaitFrame,
				p2_event,
				ISP_UsToJiffies(param.timeoutIns * 1000000));
			if (restTime == 0) {
#define ERR_STR1 "Dequeue Buffer fail, rT(%d), idx(%d), pty(%d),"
#define ERR_STR2 " pID(0x%x),cID(0x%x)\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					restTime,
					idx,
					property,
					param.processID,
					param.callerID);
#undef ERR_STR1
#undef ERR_STR2
				ret =  -EFAULT;
				break;
			}
			if (restTime == -512) {
				LOG_ERR("be stopped, restime(%d)", restTime);
				ret =  -EFAULT;
				break;
			}
		}
#define TMP_STR1 "Dequeue Buffer ok, rT(%d), idx(%d), pty(%d),"
#define TMP_STR2 " pID(0x%x), cID(0x%x)\n"
		LOG_DBG(TMP_STR1 TMP_STR2,
			restTime, idx, property, param.processID,
			param.callerID);
#undef TMP_STR1
#undef TMP_STR2
		spin_lock(&(SpinLock_P2FrameList));
		ISP_P2_BufQue_Erase(property, ISP_P2_BUFQUE_LIST_TAG_PACKAGE,
			idx);
		spin_unlock(&(SpinLock_P2FrameList));


		break;
	case ISP_P2_BUFQUE_CTRL_WAKE_WAITFRAME:
		/* wake all slept users to check buffer is dequeued or not */
		wake_up_interruptible_all(&P2WaitQueueHead_WaitFrame);
		break;
	case ISP_P2_BUFQUE_CTRL_CLAER_ALL:
		/* free all recored dequeued buffer */
		spin_lock(&(SpinLock_P2FrameList));
		for (q = 0; q < ISP_P2_BUFQUE_PROPERTY_NUM; q++) {
			for (i = 0; i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
				P2_FrameUnit_List[q][i].processID = 0x0;
				P2_FrameUnit_List[q][i].callerID = 0x0;
				P2_FrameUnit_List[q][i].cqMask = 0x0;
				P2_FrameUnit_List[q][i].bufSts =
					ISP_P2_BUF_STATE_NONE;
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
 ******************************************************************************/
static int ISP_REGISTER_IRQ_USERKEY(char *userName)
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
			/* index 0 is for all the users that do not register
			 * irq first.
			 */
			if (strcmp((void *)IrqUserKey_UserInfo[i].userName,
				userName) == 0) {
				key = IrqUserKey_UserInfo[i].userKey;
				break;
			}
		}

		/* 3. return new userkey for user if the user had not
		 * registered before.
		 */
		if (key < 0) {
			memset((void *)IrqUserKey_UserInfo[i].userName, 0,
				sizeof(IrqUserKey_UserInfo[i].userName));
			strncpy((void *)IrqUserKey_UserInfo[i].userName,
				userName, USERKEY_STR_LEN-1);
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
 ******************************************************************************/
static int ISP_MARK_IRQ(struct ISP_WAIT_IRQ_STRUCT *irqinfo)
{
	unsigned long flags; /* old: unsigned int flags;*/
	int idx = my_get_pow_idx(irqinfo->EventInfo.Status);

	unsigned long long  sec = 0;
	unsigned long       usec = 0;

	if (irqinfo->Type >= ISP_IRQ_TYPE_AMOUNT) {
		LOG_ERR("MARK_IRQ: type error(%d)", irqinfo->Type);
		return -EFAULT;
	}

	if (irqinfo->EventInfo.UserKey >= IRQ_USER_NUM_MAX ||
		irqinfo->EventInfo.UserKey < 0) {
		LOG_ERR("MARK_IRQ: userkey error(%d)",
			irqinfo->EventInfo.UserKey);
		return -EFAULT;
	}

	if (irqinfo->EventInfo.St_type == SIGNAL_INT) {
		/* 1. enable marked flag */
		spin_lock_irqsave(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);
		IspInfo.IrqInfo.MarkedFlag[irqinfo->Type][
			irqinfo->EventInfo.St_type][
			irqinfo->EventInfo.UserKey] |=
			irqinfo->EventInfo.Status;

		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[irqinfo->Type]),
			flags);

		/* 2. record mark time */
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

		spin_lock_irqsave(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);
		IspInfo.IrqInfo.MarkedTime_usec[irqinfo->Type][idx][
			irqinfo->EventInfo.UserKey] = (unsigned int)usec;

		IspInfo.IrqInfo.MarkedTime_sec[irqinfo->Type][idx][
			irqinfo->EventInfo.UserKey] = (unsigned int)sec;

		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[irqinfo->Type]),
			flags);

		/* 3. clear passed by signal count */
		spin_lock_irqsave(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);
		IspInfo.IrqInfo.PassedBySigCnt[irqinfo->Type][idx][
			irqinfo->EventInfo.UserKey] = 0;

		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[irqinfo->Type]),
			flags);

		LOG_VRB("[MARK] key/type/sts/idx (%d/%d/0x%x/%d), t(%d/%d)\n",
			irqinfo->EventInfo.UserKey, irqinfo->Type,
			irqinfo->EventInfo.Status, idx, (int)sec, (int)usec);

	} else {
#define WRN_STR1 "Not support DMA interrupt type(%d),"
#define WRN_STR2 " Only support signal interrupt!!!\n"
		LOG_WRN(WRN_STR1 WRN_STR2, irqinfo->EventInfo.St_type);
#undef WRN_STR1
#undef WRN_STR2
	}

	return 0;
}


/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_GET_MARKtoQEURY_TIME(struct ISP_WAIT_IRQ_STRUCT *irqinfo)
{
	int Ret = 0;

	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_FLUSH_IRQ(struct ISP_WAIT_IRQ_STRUCT *irqinfo)
{
	unsigned long flags; /* old: unsigned int flags;*/

	LOG_INF("type(%d)userKey(%d)St_type(%d)St(0x%x)",
		irqinfo->Type, irqinfo->EventInfo.UserKey,
		irqinfo->EventInfo.St_type, irqinfo->EventInfo.Status);

	if (irqinfo->Type >= ISP_IRQ_TYPE_AMOUNT) {
		LOG_ERR("FLUSH_IRQ: type error(%d)", irqinfo->Type);
		return -EFAULT;
	}

	if (irqinfo->EventInfo.St_type >= ISP_IRQ_ST_AMOUNT) {
		LOG_ERR("FLUSH_IRQ: st_type error(%d)",
			irqinfo->EventInfo.St_type);
		return -EFAULT;
	}

	if (irqinfo->EventInfo.UserKey >= IRQ_USER_NUM_MAX ||
		irqinfo->EventInfo.UserKey < 0) {
		LOG_ERR("FLUSH_IRQ: userkey error(%d)",
			irqinfo->EventInfo.UserKey);
		return -EFAULT;
	}

	/* 1. enable signal */
	spin_lock_irqsave(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);
	IspInfo.IrqInfo.Status[irqinfo->Type][irqinfo->EventInfo.St_type][
		irqinfo->EventInfo.UserKey] |= irqinfo->EventInfo.Status;
	spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[irqinfo->Type]), flags);

	/* 2. force to wake up the user that are waiting for that signal */
	wake_up_interruptible(&IspInfo.WaitQueueHead[irqinfo->Type]);

	return 0;
}


/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_WaitIrq(struct ISP_WAIT_IRQ_STRUCT *WaitIrq)
{

	int Ret = 0, Timeout = WaitIrq->EventInfo.Timeout;

	/*    unsigned int i;*/
	unsigned long flags; /* old: unsigned int flags;*/

	unsigned int irqStatus;
	/*    int cnt = 0;*/
	int idx = my_get_pow_idx(WaitIrq->EventInfo.Status);
	struct timeval time_getrequest;
	struct timeval time_ready2return;
	bool freeze_passbysigcnt = false;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;

	/* do_gettimeofday(&time_getrequest); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_getrequest.tv_usec = usec;
	time_getrequest.tv_sec = sec;

	if (WaitIrq->Type >= ISP_IRQ_TYPE_AMOUNT) {
		LOG_ERR("WaitIrq: type error(%d)", WaitIrq->Type);
		return -EFAULT;
	}

	if (WaitIrq->EventInfo.St_type >= ISP_IRQ_ST_AMOUNT) {
		LOG_ERR("WaitIrq: st_type error(%d)",
			WaitIrq->EventInfo.St_type);
		return -EFAULT;
	}

	if (WaitIrq->EventInfo.UserKey >= IRQ_USER_NUM_MAX ||
		WaitIrq->EventInfo.UserKey < 0) {
		LOG_ERR("WaitIrq: userkey error(%d)",
			WaitIrq->EventInfo.UserKey);
		return -EFAULT;
	}

#ifdef ENABLE_WAITIRQ_LOG
	/* Debug interrupt */
	if (IspInfo.DebugMask & ISP_DBG_INT) {
		if (WaitIrq->EventInfo.Status &
			IspInfo.IrqInfo.Mask[WaitIrq->Type][
			WaitIrq->EventInfo.St_type]) {
			if (WaitIrq->EventInfo.UserKey > 0) {
#define TMP_STR1 "+WaitIrq Clear(%d), Type(%d), Status(0x%08X),"
#define TMP_STR2 " Timeout(%d/%d),user(%d)\n"
				LOG_DBG(TMP_STR1 TMP_STR2,
					WaitIrq->EventInfo.Clear,
					WaitIrq->Type,
					WaitIrq->EventInfo.Status,
					Timeout, WaitIrq->EventInfo.Timeout,
					WaitIrq->EventInfo.UserKey);
#undef TMP_STR1
#undef TMP_STR2
			}
		}
	}
#endif

	/* 1. wait type update */
	if (WaitIrq->EventInfo.Clear == ISP_IRQ_CLEAR_STATUS) {
		spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);

		IspInfo.IrqInfo.Status[WaitIrq->Type][
			WaitIrq->EventInfo.St_type][
			WaitIrq->EventInfo.UserKey] &=
			(~WaitIrq->EventInfo.Status);

		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
		return Ret;
	}

	spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
	if (WaitIrq->EventInfo.Status &
		IspInfo.IrqInfo.MarkedFlag[WaitIrq->Type][
		WaitIrq->EventInfo.St_type][WaitIrq->EventInfo.UserKey]) {
		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
		/* force to be non_clear wait if marked before, and check the
		 * request wait timing.
		 */
		/* if the entry time of wait request after mark is before
		 * signal, we freese the counting for passby signal.
		 */
		/*  */
		/* v : kernel receive mark request */
		/* o : kernel receive wait request */
		/* : return to user */
		/*  */
		/* case: freeze is true, and passby signal count = 0 */
		/*  */
		/* |                                              | */
		/* |                                  (wait)    | */
		/* |       v-------------o++++++ | */
		/* |                                              | */
		/* Sig                                            Sig */
		/*  */
		/* case: freeze is false, and passby signal count = 1 */
		/*  */
		/* |                                              | */
		/* |                                              | */
		/* |       v---------------------- |-o  (return) */
		/* |                                              | */
		/* Sig                                            Sig */
		/*  */

		freeze_passbysigcnt = !(ISP_GetIRQState(WaitIrq->Type,
			WaitIrq->EventInfo.St_type,
			WaitIrq->EventInfo.UserKey, WaitIrq->EventInfo.Status));
	} else {
		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
			flags);

		if (WaitIrq->EventInfo.Clear == ISP_IRQ_CLEAR_WAIT) {
			spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
				flags);
			if (IspInfo.IrqInfo.Status[WaitIrq->Type][
				WaitIrq->EventInfo.St_type][
				WaitIrq->EventInfo.UserKey] &
				WaitIrq->EventInfo.Status)
				IspInfo.IrqInfo.Status[WaitIrq->Type][
					WaitIrq->EventInfo.St_type][
					WaitIrq->EventInfo.UserKey] &=
					(~WaitIrq->EventInfo.Status);

			spin_unlock_irqrestore(
				&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
		} else if (WaitIrq->EventInfo.Clear == ISP_IRQ_CLEAR_ALL) {
			spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
				flags);

			IspInfo.IrqInfo.Status[WaitIrq->Type][
				WaitIrq->EventInfo.St_type][
				WaitIrq->EventInfo.UserKey] = 0;
			spin_unlock_irqrestore(
				&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
		}
	}


	/* Store irqinfo status here to reduce time of spin_lock_irqsave. */
	spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
	irqStatus = IspInfo.IrqInfo.Status[WaitIrq->Type][
		WaitIrq->EventInfo.St_type][WaitIrq->EventInfo.UserKey];
	spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);

	if (WaitIrq->EventInfo.Clear == ISP_IRQ_CLEAR_NONE) {
		if (IspInfo.IrqInfo.Status[WaitIrq->Type][
			WaitIrq->EventInfo.St_type][
			WaitIrq->EventInfo.UserKey] &
			WaitIrq->EventInfo.Status) {
#ifdef ENABLE_WAITIRQ_LOG
#define TMP_STR1 "Already have irq!!!: WaitIrq Timeout(%d) Clear(%d), Type(%d),"
#define TMP_STR2 " StType(%d), IrqStatus(0x%08X), WaitStatus(0x%08X),"
#define TMP_STR3 " Timeout(%d), userKey(%d)\n"
			LOG_INF(TMP_STR1 TMP_STR2 TMP_STR3,
				WaitIrq->EventInfo.Timeout,
				WaitIrq->EventInfo.Clear,
				WaitIrq->Type,
				WaitIrq->EventInfo.St_type,
				irqStatus,
				WaitIrq->EventInfo.Status,
				WaitIrq->EventInfo.Timeout,
				WaitIrq->EventInfo.UserKey);
#undef TMP_STR1
#undef TMP_STR2
#undef TMP_STR3
			}
#endif

			goto NON_CLEAR_WAIT;
		}
	}
#ifdef ENABLE_WAITIRQ_LOG
#define TMP_STR1 "before wait: Clear(%d) Type(%d) StType(%d) Sts(0x%08X)"
#define TMP_STR2 " WaitSts(0x%08X) Timeout(%d) userKey(%d)\n"
	LOG_INF(TMP_STR1 TMP_STR2,
		WaitIrq->EventInfo.Clear,
		WaitIrq->Type,
		WaitIrq->EventInfo.St_type,
		irqStatus,
		WaitIrq->EventInfo.Status,
		WaitIrq->EventInfo.Timeout,
		WaitIrq->EventInfo.UserKey);
#undef TMP_STR1
#undef TMP_STR2
#endif

	/* 2. start to wait signal */
	Timeout = wait_event_interruptible_timeout(
		IspInfo.WaitQueueHead[WaitIrq->Type],
		ISP_GetIRQState(WaitIrq->Type, WaitIrq->EventInfo.St_type,
			WaitIrq->EventInfo.UserKey,
			WaitIrq->EventInfo.Status),
		ISP_MsToJiffies(WaitIrq->EventInfo.Timeout));

	/* check if user is interrupted by system signal */
	if ((Timeout != 0) &&
		(!ISP_GetIRQState(WaitIrq->Type, WaitIrq->EventInfo.St_type,
		WaitIrq->EventInfo.UserKey, WaitIrq->EventInfo.Status))) {
#define TMP_STR1 "interrupted by system signal, return value(%d),"
#define TMP_STR2 " irq Type/User/Sts(0x%x/%d/0x%x)\n"

		LOG_INF(TMP_STR1 TMP_STR2,
			Timeout, WaitIrq->Type, WaitIrq->EventInfo.UserKey,
			WaitIrq->EventInfo.Status);
#undef TMP_STR1
#undef TMP_STR2
		Ret = -SIG_ERESTARTSYS; /* actually it should be -ERESTARTSYS */
		goto EXIT;
	} else if (Timeout == 0) { /* timeout */
		/* Store irqinfo status here to reduce time of
		 * spin_lock_irqsave.
		 */
		spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = IspInfo.IrqInfo.Status[WaitIrq->Type][
			WaitIrq->EventInfo.St_type][WaitIrq->EventInfo.UserKey];
		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
#define ERR_STR1 "ERROR WaitIrq Clear(%d) Type(%d) StType(%d) Status(0x%08X)"
#define ERR_STR2 " WaitStatus(0x%08X) Timeout(%d) key(%d)\n"
		LOG_ERR(ERR_STR1 ERR_STR2,
			WaitIrq->EventInfo.Clear,
			WaitIrq->Type,
			WaitIrq->EventInfo.St_type,
			irqStatus,
			WaitIrq->EventInfo.Status,
			WaitIrq->EventInfo.Timeout,
			WaitIrq->EventInfo.UserKey);
#undef ERR_STR1
#undef ERR_STR2

		if (WaitIrq->bDumpReg)
			ISP_DumpReg();

		if (WaitIrq->bDumpReg &&
			((WaitIrq->EventInfo.Status == SOF_INT_ST) ||
			 (WaitIrq->EventInfo.Status == SW_PASS1_DON_ST))) {
			ISP_DumpSeninfReg();
		}
		Ret = -EFAULT;
		goto EXIT;
	}
#ifdef ENABLE_WAITIRQ_LOG
	else {
		/* Store irqinfo status here to reduce time of
		 * spin_lock_irqsave.
		 */
		spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus =
			IspInfo.IrqInfo.Status[WaitIrq->Type][
			WaitIrq->EventInfo.St_type][WaitIrq->EventInfo.UserKey];
		spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
#define TMP_STR1 "Done WaitIrq Clear(%d) Type(%d) StType(%d) Status(0x%08X)"
#define TMP_STR2 " WaitStatus(0x%08X) Timeout(%d) key(%d)\n"
		LOG_INF(TMP_STR1 TMP_STR2,
			WaitIrq->EventInfo.Clear,
			WaitIrq->Type,
			WaitIrq->EventInfo.St_type,
			irqStatus,
			WaitIrq->EventInfo.Status,
			WaitIrq->EventInfo.Timeout,
			WaitIrq->EventInfo.UserKey);
#undef TMP_STR1
#undef TMP_STR2
	}
#endif

NON_CLEAR_WAIT:
	/* 3. get interrupt and update time related information that would
	 * be return to user.
	 */
	/* do_gettimeofday(&time_ready2return); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_ready2return.tv_usec = usec;
	time_ready2return.tv_sec = sec;


	spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);

	/* clear the status if someone get the irq */
	IspInfo.IrqInfo.Status[WaitIrq->Type][WaitIrq->EventInfo.St_type][
		WaitIrq->EventInfo.UserKey] &= (~WaitIrq->EventInfo.Status);
	spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);


EXIT:
	/* 4. clear mark flag / reset marked time / reset time related infor
	 * and passedby signal count.
	 */
	spin_lock_irqsave(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);
	if (WaitIrq->EventInfo.Status &
		IspInfo.IrqInfo.MarkedFlag[WaitIrq->Type][
		WaitIrq->EventInfo.St_type][WaitIrq->EventInfo.UserKey]) {

		IspInfo.IrqInfo.MarkedFlag[WaitIrq->Type][
			WaitIrq->EventInfo.St_type][
			WaitIrq->EventInfo.UserKey] &=
			(~WaitIrq->EventInfo.Status);

		IspInfo.IrqInfo.MarkedTime_usec[
			WaitIrq->Type][idx][WaitIrq->EventInfo.UserKey] = 0;
		IspInfo.IrqInfo.MarkedTime_sec[
			WaitIrq->Type][idx][WaitIrq->EventInfo.UserKey] = 0;
		IspInfo.IrqInfo.PassedBySigCnt[
			WaitIrq->Type][idx][WaitIrq->EventInfo.UserKey] = 0;
	}
	spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[WaitIrq->Type]), flags);


	return Ret;
}


#ifdef ENABLE_KEEP_ION_HANDLE
/*******************************************************************************
 *
 ******************************************************************************/
static void ISP_ion_init(void)
{
	if (!pIon_client && g_ion_device)
		pIon_client = ion_client_create(g_ion_device, "camera_isp");

	if (!pIon_client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}

	if (IspInfo.DebugMask & ISP_DBG_ION_CTRL)
		LOG_INF("create ion client 0x%p\n", pIon_client);
}

/*******************************************************************************
 *
 ******************************************************************************/
static void ISP_ion_uninit(void)
{
	if (!pIon_client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}

	if (IspInfo.DebugMask & ISP_DBG_ION_CTRL)
		LOG_INF("destroy ion client 0x%p\n", pIon_client);

	ion_client_destroy(pIon_client);

	pIon_client = NULL;
}

/*******************************************************************************
 *
 ******************************************************************************/
static struct ion_handle *ISP_ion_import_handle(struct ion_client *client,
	int fd)
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

	handle = ion_import_dma_buf_fd(client, fd);

	if (IS_ERR(handle)) {
		LOG_ERR("import ion handle failed!\n");
		return NULL;
	}

	if (IspInfo.DebugMask & ISP_DBG_ION_CTRL)
		LOG_INF("[ion_import_hd] Hd(0x%p)\n", handle);
	return handle;
}

/*******************************************************************************
 *
 ******************************************************************************/
static void ISP_ion_free_handle(struct ion_client *client,
	struct ion_handle *handle)
{
	if (!client) {
		LOG_ERR("invalid ion client!\n");
		return;
	}
	if (!handle)
		return;

	if (IspInfo.DebugMask & ISP_DBG_ION_CTRL)
		LOG_INF("[ion_free_hd] Hd(0x%p)\n", handle);

	ion_free(client, handle);

}

/*******************************************************************************
 *
 ******************************************************************************/
static void ISP_ion_free_handle_by_module(unsigned int module)
{
	int i, j;
	int nFd;
	struct ion_handle *p_IonHnd;
	struct T_ION_TBL *ptbl = &gION_TBL[module];

	if (IspInfo.DebugMask & ISP_DBG_ION_CTRL)
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
			if (IspInfo.DebugMask & ISP_DBG_ION_CTRL) {
#define TMP_STR "ion_free: dev(%d)dma(%d)j(%d)fd(%d)Hnd(0x%p)\n"
				LOG_INF(TMP_STR,
					module, i, j, nFd, p_IonHnd);
#undef TMP_STR
			}
			ISP_ion_free_handle(pIon_client, p_IonHnd);
		}
	}
}

#endif


/*******************************************************************************
 *
 ******************************************************************************/
static long ISP_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	int Ret = 0;
	/*  */
	/*    bool   HoldEnable = MFALSE;*/
	unsigned int DebugFlag[3] = {0};
	/*    unsigned int pid = 0;*/
	struct ISP_REG_IO_STRUCT       RegIo;
	struct ISP_DUMP_BUFFER_STRUCT DumpBufStruct;
	struct ISP_MEM_INFO_STRUCT MemInfoStruct;
	struct ISP_WAIT_IRQ_STRUCT     IrqInfo;
	struct ISP_CLEAR_IRQ_STRUCT    ClearIrq;
	struct ISP_USER_INFO_STRUCT *pUserInfo;
	struct ISP_P2_BUFQUE_STRUCT    p2QueBuf;
	unsigned int                 regScenInfo_value = 0xa5a5a5a5;
	/*    int                  burstQNum;*/
	unsigned int                 wakelock_ctrl;
	unsigned int                 module;
	unsigned long flags; /* old: unsigned int flags;*/
	int userKey =  -1;
	struct ISP_REGISTER_USERKEY_STRUCT RegUserKey;
	int i;
	#ifdef ENABLE_KEEP_ION_HANDLE
	struct ISP_DEV_ION_NODE_STRUCT IonNode;
	struct ion_handle *handle;
	struct ion_handle *p_IonHnd;
	#endif
	struct ISP_CLK_INFO ispclks;
	unsigned int lv = 0;

	/*  */
	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(proc, pid, tgid)=(%s, %d, %d)\n",
			current->comm, current->pid, current->tgid);
		return -EFAULT;
	}
	/*  */
	pUserInfo = (struct ISP_USER_INFO_STRUCT *)(pFile->private_data);
	/*  */
	switch (Cmd) {
	case ISP_WAKELOCK_CTRL:
		if (copy_from_user(&wakelock_ctrl, (void *)Param,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("get ISP_WAKELOCK_CTRL from user fail\n");
			Ret = -EFAULT;
		} else {
			if (wakelock_ctrl == 1) {       /* Enable wakelock */
				if (g_WaitLockCt) {
					g_WaitLockCt++;
					LOG_DBG("add wakelock cnt(%d)\n",
						g_WaitLockCt);
				} else {
#ifdef CONFIG_PM_WAKELOCKS
					__pm_stay_awake(&isp_wake_lock);
#else
					wake_lock(&isp_wake_lock);
#endif
					g_WaitLockCt++;
					LOG_DBG("wakelock enable!! cnt(%d)\n",
						g_WaitLockCt);
				}
			} else {        /* Disable wakelock */
				if (g_WaitLockCt)
					g_WaitLockCt--;

				if (g_WaitLockCt)
					LOG_DBG("subtract wakelock cnt(%d)\n",
						g_WaitLockCt);
				else {
#ifdef CONFIG_PM_WAKELOCKS
					__pm_relax(&isp_wake_lock);
#else
					wake_unlock(&isp_wake_lock);
#endif
					LOG_DBG("wakelock disable!! cnt(%d)\n",
						g_WaitLockCt);
				}
			}
		}
		break;
	case ISP_GET_DROP_FRAME:
		if (copy_from_user(&DebugFlag[0], (void *)Param,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("get irq from user fail\n");
			Ret = -EFAULT;
		} else {
			switch (DebugFlag[0]) {
			case ISP_IRQ_TYPE_INT_CAM_A_ST:
				spin_lock_irqsave(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAM_A_ST]), flags);
				DebugFlag[1] =
					FrameStatus[ISP_IRQ_TYPE_INT_CAM_A_ST];
				spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAM_A_ST]), flags);
				break;
			case ISP_IRQ_TYPE_INT_CAM_B_ST:
				spin_lock_irqsave(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAM_B_ST]), flags);
				DebugFlag[1] =
					FrameStatus[ISP_IRQ_TYPE_INT_CAM_B_ST];
				spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAM_B_ST]), flags);
				break;
			case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
				spin_lock_irqsave(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAMSV_0_ST]), flags);
				DebugFlag[1] = FrameStatus[
					ISP_IRQ_TYPE_INT_CAMSV_0_ST];
				spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAMSV_0_ST]), flags);
				break;
			case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
				spin_lock_irqsave(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAMSV_1_ST]), flags);
				DebugFlag[1] = FrameStatus[
					ISP_IRQ_TYPE_INT_CAMSV_1_ST];
				spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAMSV_1_ST]), flags);
				break;
			default:
				LOG_ERR("err TG(0x%x)\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
			if (copy_to_user((void *)Param, &DebugFlag[1],
				sizeof(unsigned int)) != 0) {
				LOG_ERR("copy to user fail\n");
				Ret = -EFAULT;
			}
		}
		break;
	case ISP_GET_INT_ERR:
		if (copy_to_user((void *)Param, (void *)g_ISPIntErr,
			sizeof(unsigned int) * ISP_IRQ_TYPE_AMOUNT) != 0)
			LOG_ERR("get int err fail\n");
		else
			memset((void *)g_ISPIntErr, 0, sizeof(g_ISPIntErr));

		break;
	case ISP_GET_DMA_ERR:
		if (copy_from_user(&DebugFlag[0], (void *)Param,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("get module fail\n");
			Ret = -EFAULT;
		} else {
			if (DebugFlag[0] >= (ISP_IRQ_TYPE_AMOUNT)) {
				LOG_ERR("module error(%d)\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
			if (copy_to_user((void *)Param,
				&g_DmaErr_CAM[DebugFlag[0]],
				sizeof(unsigned int)*_cam_max_) != 0)
				LOG_ERR("get dma_err fail\n");

		}
		break;
	case ISP_GET_CUR_SOF:
		if (copy_from_user(&DebugFlag[0], (void *)Param,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("get cur sof from user fail\n");
			Ret = -EFAULT;
		} else {
#if 0
			switch (DebugFlag[0]) {
			case ISP_IRQ_TYPE_INT_CAM_A_ST:
				DebugFlag[1] = ISP_RD32_TG_CAM_FRM_CNT(
					ISP_IRQ_TYPE_INT_CAM_A_ST,
					ISP_CAM_A_IDX);
				break;
			case ISP_IRQ_TYPE_INT_CAM_B_ST:
				DebugFlag[1] = ISP_RD32_TG_CAM_FRM_CNT(
					ISP_IRQ_TYPE_INT_CAM_B_ST,
					ISP_CAM_B_IDX));
				break;
			case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
				DebugFlag[1] = ISP_RD32_TG_CAM_FRM_CNT(
					ISP_IRQ_TYPE_INT_CAMSV_0_ST,
					ISP_CAMSV0_IDX);
				break;
			case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
				DebugFlag[1] = ISP_RD32_TG_CAM_FRM_CNT(
					ISP_IRQ_TYPE_INT_CAMSV_1_ST,
					ISP_CAMSV1_IDX);
				break;
			default:
				LOG_ERR("err TG(0x%x)\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
#else
			if (DebugFlag[0] >= ISP_IRQ_TYPE_AMOUNT) {
				LOG_ERR("cursof: error type(%d)\n",
					DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
			DebugFlag[1] = sof_count[DebugFlag[0]];
#endif
		}
		if (copy_to_user((void *)Param, &DebugFlag[1],
			sizeof(unsigned int)) != 0) {
			LOG_ERR("copy to user fail\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_RESET_BY_HWMODULE: {
		if (copy_from_user(&module, (void *)Param,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("get hwmodule from user fail\n");
			Ret = -EFAULT;
		} else {
			ISP_Reset(module);
		}
		break;
	}
	case ISP_READ_REGISTER: {
		if (copy_from_user(&RegIo, (void *)Param,
			sizeof(struct ISP_REG_IO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented
			 * in ISP_ReadReg(...)
			 */
			Ret = ISP_ReadReg(&RegIo);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case ISP_WRITE_REGISTER: {
		if (copy_from_user(&RegIo, (void *)Param,
			sizeof(struct ISP_REG_IO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented
			 * in ISP_WriteReg(...)
			 */
			Ret = ISP_WriteReg(&RegIo);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case ISP_WAIT_IRQ: {
		if (copy_from_user(&IrqInfo, (void *)Param,
			sizeof(struct ISP_WAIT_IRQ_STRUCT)) == 0) {
			/*  */
			if ((IrqInfo.Type >= ISP_IRQ_TYPE_AMOUNT) ||
				(IrqInfo.Type < 0)) {
				Ret = -EFAULT;
				LOG_ERR("invalid type(%d)\n", IrqInfo.Type);
				goto EXIT;
			}

			if ((IrqInfo.EventInfo.St_type >= ISP_IRQ_ST_AMOUNT) ||
				(IrqInfo.EventInfo.St_type < 0)) {
#define ERR_STR "invalid St_type(%d), max(%d), force St_type = 0\n"
				LOG_ERR(ERR_STR, IrqInfo.EventInfo.St_type,
					ISP_IRQ_ST_AMOUNT);
#undef ERR_STR
				IrqInfo.EventInfo.St_type = 0;
			}

			if ((IrqInfo.EventInfo.UserKey >= IRQ_USER_NUM_MAX) ||
				(IrqInfo.EventInfo.UserKey < 0)) {
#define ERR_STR "invalid userKey(%d), max(%d), force userkey = 0\n"
				LOG_ERR(ERR_STR,
					IrqInfo.EventInfo.UserKey,
					IRQ_USER_NUM_MAX);
#undef ERR_STR
				IrqInfo.EventInfo.UserKey = 0;
			}
#ifdef ENABLE_WAITIRQ_LOG
#define TMP_STR1 "IRQ type(%d), userKey(%d), timeout(%d), userkey(%d),"
#define TMP_STR2 " st_status(%d), status(%d)\n"
			LOG_INF(TMP_STR1 TMP_STR2,
				IrqInfo.Type, IrqInfo.EventInfo.UserKey,
				IrqInfo.EventInfo.Timeout,
				IrqInfo.EventInfo.UserKey,
				IrqInfo.EventInfo.St_type,
				IrqInfo.EventInfo.Status);
#undef TMP_STR1
#undef TMP_STR2
#endif
			Ret = ISP_WaitIrq(&IrqInfo);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case ISP_CLEAR_IRQ: {
		if (copy_from_user(&ClearIrq, (void *)Param,
			sizeof(struct ISP_CLEAR_IRQ_STRUCT)) == 0) {
			LOG_DBG("ISP_CLEAR_IRQ Type(%d)\n", ClearIrq.Type);

			if ((ClearIrq.Type >= ISP_IRQ_TYPE_AMOUNT) ||
				(ClearIrq.Type < 0)) {
				Ret = -EFAULT;
				LOG_ERR("invalid type(%d)\n", ClearIrq.Type);
				goto EXIT;
			}

			if ((ClearIrq.EventInfo.St_type >= ISP_IRQ_ST_AMOUNT) ||
				(ClearIrq.EventInfo.St_type < 0)) {
#define TMP_STR "invalid St_type(%d), max(%d), force St_type = 0\n"
				LOG_ERR(TMP_STR,
					ClearIrq.EventInfo.St_type,
					ISP_IRQ_ST_AMOUNT);
#undef TMP_STR
				ClearIrq.EventInfo.St_type = 0;
			}

			/*  */
			if ((ClearIrq.EventInfo.UserKey >= IRQ_USER_NUM_MAX) ||
				(ClearIrq.EventInfo.UserKey < 0)) {
				LOG_ERR("errUserEnum(%d)",
					ClearIrq.EventInfo.UserKey);
				Ret = -EFAULT;
				goto EXIT;
			}

			/*avoid line over 80 char per line */
			i = ClearIrq.EventInfo.UserKey;
#define TMP_STR1 "ISP_CLEAR_IRQ:Type(%d), Status(0x%x), st_status(%d),"
#define TMP_STR2 " IrqStatus(0x%x)\n"
			LOG_DBG(TMP_STR1 TMP_STR2,
				ClearIrq.Type, ClearIrq.EventInfo.Status,
				ClearIrq.EventInfo.St_type,
				IspInfo.IrqInfo.Status[ClearIrq.Type][
				ClearIrq.EventInfo.St_type][i]);
#undef TMP_STR1
#undef TMP_STR2
			spin_lock_irqsave(&(IspInfo.SpinLockIrq[ClearIrq.Type]),
				flags);
			IspInfo.IrqInfo.Status[ClearIrq.Type][
				ClearIrq.EventInfo.St_type][
				ClearIrq.EventInfo.UserKey] &=
				(~ClearIrq.EventInfo.Status);
			spin_unlock_irqrestore(
				&(IspInfo.SpinLockIrq[ClearIrq.Type]), flags);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	/*  */
	case ISP_REGISTER_IRQ_USER_KEY:
		if (copy_from_user(&RegUserKey, (void *)Param,
			sizeof(struct ISP_REGISTER_USERKEY_STRUCT)) == 0) {
			userKey = ISP_REGISTER_IRQ_USERKEY(RegUserKey.userName);
			RegUserKey.userKey = userKey;
			if (copy_to_user((void *)Param, &RegUserKey, sizeof(
				struct ISP_REGISTER_USERKEY_STRUCT)) != 0)
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
	case ISP_MARK_IRQ_REQUEST:
		if (copy_from_user(&IrqInfo, (void *)Param,
			sizeof(struct ISP_WAIT_IRQ_STRUCT)) == 0) {
			if ((IrqInfo.EventInfo.UserKey >= IRQ_USER_NUM_MAX) ||
				(IrqInfo.EventInfo.UserKey < 1)) {
				LOG_ERR("invalid userKey(%d), max(%d)\n",
					IrqInfo.EventInfo.UserKey,
					IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((IrqInfo.Type >= ISP_IRQ_TYPE_AMOUNT) ||
				(IrqInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)\n",
					IrqInfo.Type, ISP_IRQ_TYPE_AMOUNT);
				Ret = -EFAULT;
				break;
			}

			if ((IrqInfo.EventInfo.St_type >= ISP_IRQ_ST_AMOUNT) ||
				(IrqInfo.EventInfo.St_type < 0)) {
#define ERR_STR "invalid St_type(%d), max(%d), force St_type = 0\n"
				LOG_ERR(ERR_STR,
					IrqInfo.EventInfo.St_type,
					ISP_IRQ_ST_AMOUNT);
#undef ERR_STR
				IrqInfo.EventInfo.St_type = 0;
			}
			Ret = ISP_MARK_IRQ(&IrqInfo);
		} else {
			LOG_ERR("ISP_MARK_IRQ, copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	/*  */
	case ISP_GET_MARK2QUERY_TIME:
		if (copy_from_user(&IrqInfo, (void *)Param,
			sizeof(struct ISP_WAIT_IRQ_STRUCT)) == 0) {
			if ((IrqInfo.EventInfo.UserKey >= IRQ_USER_NUM_MAX) ||
				(IrqInfo.EventInfo.UserKey < 1)) {
				LOG_ERR("invalid userKey(%d), max(%d)\n",
					IrqInfo.EventInfo.UserKey,
					IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((IrqInfo.Type >= ISP_IRQ_TYPE_AMOUNT) ||
				(IrqInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)\n",
					IrqInfo.Type, ISP_IRQ_TYPE_AMOUNT);
				Ret = -EFAULT;
				break;
			}
			Ret = ISP_GET_MARKtoQEURY_TIME(&IrqInfo);
			/*  */
			if (copy_to_user((void *)Param, &IrqInfo,
				sizeof(struct ISP_WAIT_IRQ_STRUCT)) != 0) {
				LOG_ERR("copy_to_user failed\n");
				Ret = -EFAULT;
			}
		} else {
#define ERR_STR "ISP_GET_MARK2QUERY_TIME, copy_from_user failed\n"
			LOG_ERR(ERR_STR);
#undef ERR_STR
			Ret = -EFAULT;
		}
		break;
	/*  */
	case ISP_FLUSH_IRQ_REQUEST:
		if (copy_from_user(&IrqInfo, (void *)Param,
			sizeof(struct ISP_WAIT_IRQ_STRUCT)) == 0) {
			if ((IrqInfo.EventInfo.UserKey >= IRQ_USER_NUM_MAX) ||
				(IrqInfo.EventInfo.UserKey < 0)) {
				LOG_ERR("invalid userKey(%d), max(%d)\n",
					IrqInfo.EventInfo.UserKey,
					IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((IrqInfo.Type >= ISP_IRQ_TYPE_AMOUNT) ||
				(IrqInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)\n",
					IrqInfo.Type, ISP_IRQ_TYPE_AMOUNT);
				Ret = -EFAULT;
				break;
			}
			if ((IrqInfo.EventInfo.St_type >= ISP_IRQ_ST_AMOUNT) ||
				(IrqInfo.EventInfo.St_type < 0)) {
#define ERR_STR "invalid St_type(%d), max(%d), force St_type = 0\n"
				LOG_ERR(ERR_STR,
					IrqInfo.EventInfo.St_type,
					ISP_IRQ_ST_AMOUNT);
#undef ERR_STR
				IrqInfo.EventInfo.St_type = 0;
			}

			Ret = ISP_FLUSH_IRQ(&IrqInfo);
		}
		break;
	/*  */
	case ISP_P2_BUFQUE_CTRL:
		if (copy_from_user(&p2QueBuf, (void *)Param,
			sizeof(struct ISP_P2_BUFQUE_STRUCT)) == 0) {
			p2QueBuf.processID = pUserInfo->Pid;
			Ret = ISP_P2_BufQue_CTRL_FUNC(p2QueBuf);
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	/*  */
	case ISP_UPDATE_REGSCEN:
		if (copy_from_user(&regScenInfo_value, (void *)Param,
			sizeof(unsigned int)) == 0) {
			spin_lock((spinlock_t *)(&SpinLockRegScen));
			g_regScen = regScenInfo_value;
			spin_unlock((spinlock_t *)(&SpinLockRegScen));
		} else {
			LOG_ERR("copy_from_user	failed\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_QUERY_REGSCEN:
		spin_lock((spinlock_t *)(&SpinLockRegScen));
		regScenInfo_value = g_regScen;
		spin_unlock((spinlock_t *)(&SpinLockRegScen));
		/*      */
		if (copy_to_user((void *)Param, &regScenInfo_value,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("copy_to_user failed\n");
			Ret = -EFAULT;
		}
		break;
	/*  */
	case ISP_UPDATE_BURSTQNUM:
		break;
	case ISP_QUERY_BURSTQNUM:
		break;
	/*  */
	case ISP_DUMP_REG:
		/* TODO: modify this... to mt6797 */
		Ret = ISP_DumpReg();
		break;
	case ISP_DEBUG_FLAG:
		if (copy_from_user(DebugFlag, (void *)Param,
			sizeof(unsigned int)) == 0) {

			IspInfo.DebugMask = DebugFlag[0];

		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_BUFFER_CTRL:
		Ret = ISP_Buf_CTRL_FUNC(Param);
		break;
	case ISP_REF_CNT_CTRL:
		Ret = ISP_REF_CNT_CTRL_FUNC(Param);
		break;
	case ISP_DUMP_ISR_LOG:
		if (copy_from_user(DebugFlag, (void *)Param,
			sizeof(unsigned int)) == 0) {
			unsigned int currentPPB = m_CurrentPPB;
			unsigned int lock_key = ISP_IRQ_TYPE_AMOUNT;

			if (DebugFlag[0] >= ISP_IRQ_TYPE_AMOUNT) {
				LOG_ERR("unsupported module:0x%x\n",
					DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}

			if (DebugFlag[0] == ISP_IRQ_TYPE_INT_CAM_B_ST)
				lock_key = ISP_IRQ_TYPE_INT_CAM_A_ST;
			else
				lock_key = DebugFlag[0];

			spin_lock_irqsave(&(IspInfo.SpinLockIrq[lock_key]),
				flags);
			m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
			spin_unlock_irqrestore(&(IspInfo.SpinLockIrq[lock_key]),
				flags);

			IRQ_LOG_PRINTER(DebugFlag[0], currentPPB, _LOG_INF);
			IRQ_LOG_PRINTER(DebugFlag[0], currentPPB, _LOG_ERR);

		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_VF_LOG:
		if (copy_from_user(DebugFlag, (void *)Param,
			sizeof(unsigned int) * 3) == 0) {
			unsigned int vf;

			switch (DebugFlag[0]) {
			case 1: {
				unsigned int module = ISP_IRQ_TYPE_INT_CAM_A_ST;
				unsigned int cam_dmao = 0;
				unsigned int uni_dmao =
					ISP_RD32(CAM_UNI_REG_TOP_DMA_EN(
						ISP_UNI_A_IDX));
				unsigned int hds2_sel =
					((ISP_RD32(CAM_UNI_REG_TOP_PATH_SEL(
						ISP_UNI_A_IDX)) >> 8) & 0x3);

				switch (DebugFlag[1]) {
				case ISP_CAM_A_IDX:
					LOG_INF("CAM_A viewFinder is ON\n");
					cam_dmao = ISP_RD32(CAM_REG_CTL_DMA_EN(
						ISP_CAM_A_IDX));
					LOG_INF("CAM_A:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAM_REG_TG_VF_CON(
						ISP_CAM_A_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR, ISP_CAM_A_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAM_REG_TG_VF_CON(
							ISP_CAM_A_IDX),
							(vf + 0x1));

					if (hds2_sel == 0) {

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_A_ST]->
						ring_buf[_eiso_].active =
						((uni_dmao & 0x4) ?
							(MTRUE) : (MFALSE));

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_A_ST]->
						ring_buf[_rsso_].active =
						((uni_dmao & 0x8) ?
							(MTRUE) : (MFALSE));

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_B_ST]->
						ring_buf[_eiso_].active =
						MFALSE;

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_B_ST]->
						ring_buf[_rsso_].active =
						MFALSE;

					}

					module = ISP_IRQ_TYPE_INT_CAM_A_ST;

					#if (TIMESTAMP_QUEUE_EN == 1)
					memset((void *)&(IspInfo.TstpQInfo[
						ISP_IRQ_TYPE_INT_CAM_A_ST]), 0,
						sizeof(struct
						ISP_TIMESTPQ_INFO_STRUCT));

					g1stSwP1Done[
						ISP_IRQ_TYPE_INT_CAM_A_ST] =
						MTRUE;

					gPrevSofTimestp[
						ISP_IRQ_TYPE_INT_CAM_A_ST] = 0;
					#endif
					break;
				case ISP_CAM_B_IDX:
					LOG_INF("CAM_B viewFinder is ON\n");
					cam_dmao = ISP_RD32(CAM_REG_CTL_DMA_EN(
						ISP_CAM_B_IDX));
					LOG_INF("CAM_B:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAM_REG_TG_VF_CON(
						ISP_CAM_B_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR, ISP_CAM_B_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAM_REG_TG_VF_CON(
							ISP_CAM_B_IDX),
							(vf + 0x1));

					if (hds2_sel == 1) {

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_A_ST]->
						ring_buf[_eiso_].active =
						MFALSE;

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_A_ST]->
						ring_buf[_rsso_].active =
						MFALSE;

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_B_ST]->
						ring_buf[_eiso_].active =
						((uni_dmao & 0x4) ?
							(MTRUE) : (MFALSE));

					pstRTBuf[ISP_IRQ_TYPE_INT_CAM_B_ST]->
						ring_buf[_rsso_].active =
						((uni_dmao & 0x8) ?
							(MTRUE) : (MFALSE));

					}

					module = ISP_IRQ_TYPE_INT_CAM_B_ST;

					#if (TIMESTAMP_QUEUE_EN == 1)
					memset((void *)&(IspInfo.TstpQInfo[
						ISP_IRQ_TYPE_INT_CAM_B_ST]), 0,
						sizeof(struct
						ISP_TIMESTPQ_INFO_STRUCT));

					g1stSwP1Done[
						ISP_IRQ_TYPE_INT_CAM_B_ST] =
						MTRUE;

					gPrevSofTimestp[
						ISP_IRQ_TYPE_INT_CAM_B_ST] = 0;
					#endif

					break;
				default:
					LOG_ERR("unsupported module:0x%x\n",
						DebugFlag[1]);
					break;
				}
				pstRTBuf[module]->ring_buf[_imgo_].active =
					((cam_dmao & 0x1) ?
						(MTRUE) : (MFALSE));
				pstRTBuf[module]->ring_buf[_rrzo_].active =
					((cam_dmao & 0x4) ?
						(MTRUE) : (MFALSE));
				pstRTBuf[module]->ring_buf[_lcso_].active =
					((cam_dmao & 0x10) ?
						(MTRUE) : (MFALSE));
				/*reset 1st sof flag when vf is enabled*/
				g1stSof[module] = MTRUE;
				break;
			}
			case 0: {
				switch (DebugFlag[1]) {
				case ISP_CAM_A_IDX:
					LOG_INF("CAM_A viewFinder is OFF\n");
					vf = ISP_RD32(CAM_REG_TG_VF_CON(
						ISP_CAM_A_IDX));
					if (vf & 0x1)
						ISP_WR32(CAM_REG_TG_VF_CON(
						ISP_CAM_A_IDX), (vf-0x1));
					else {
#define ERR_STR "module_%d: vf already disabled\n"
						LOG_ERR(ERR_STR, ISP_CAM_A_IDX);
#undef ERR_STR
					}
					break;
				case ISP_CAM_B_IDX:
					LOG_INF("CAM_B viewFinder is OFF\n");
					vf = ISP_RD32(CAM_REG_TG_VF_CON(
						ISP_CAM_B_IDX));
					if (vf & 0x1)
						ISP_WR32(CAM_REG_TG_VF_CON(
							ISP_CAM_B_IDX),
							(vf - 0x1));
					else {
#define ERR_STR "module_%d: vf already disabled\n"
						LOG_ERR(ERR_STR, ISP_CAM_B_IDX);
#undef ERR_STR
					}
					break;
				default:
					LOG_ERR("unsupported module:0x%x\n",
						DebugFlag[1]);
					break;
				}
				break;
			}
			/* CAMSV */
			case 11: {
				unsigned int module =
					ISP_IRQ_TYPE_INT_CAMSV_0_ST;
				unsigned int cam_dmao = 0;

				switch (DebugFlag[1]) {
				case ISP_CAMSV0_IDX:
					LOG_INF("CAMSV_0 viewFinder is ON\n");
					cam_dmao =
						(ISP_RD32(CAMSV_REG_MODULE_EN(
						ISP_CAMSV0_IDX)) & 0x10);
					LOG_INF("CAMSV_0:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV0_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR,
							ISP_CAMSV0_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV0_IDX),
							(vf + 0x1));
					module = ISP_IRQ_TYPE_INT_CAMSV_0_ST;
					break;
				case ISP_CAMSV1_IDX:
					LOG_INF("CAMSV_1 viewFinder is ON\n");
					cam_dmao =
						(ISP_RD32(CAMSV_REG_MODULE_EN(
						ISP_CAMSV1_IDX)) & 0x10);
					LOG_INF("CAMSV_1:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV1_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR,
							ISP_CAMSV1_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV1_IDX),
							(vf + 0x1));
					module = ISP_IRQ_TYPE_INT_CAMSV_0_ST;
					break;
				case ISP_CAMSV2_IDX:
					LOG_INF("CAMSV_2 viewFinder is ON\n");
					cam_dmao =
						(ISP_RD32(CAMSV_REG_MODULE_EN(
						ISP_CAMSV2_IDX)) & 0x10);
					LOG_INF("CAMSV_2:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV2_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR,
							ISP_CAMSV2_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV2_IDX),
							(vf+0x1));
					module = ISP_IRQ_TYPE_INT_CAMSV_2_ST;
					break;
				case ISP_CAMSV3_IDX:
					LOG_INF("CAMSV_3 viewFinder is ON\n");
					cam_dmao =
						(ISP_RD32(CAMSV_REG_MODULE_EN(
						ISP_CAMSV3_IDX)) & 0x10);
					LOG_INF("CAMSV_3:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV3_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR,
							ISP_CAMSV3_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV3_IDX),
							(vf + 0x1));
					module = ISP_IRQ_TYPE_INT_CAMSV_3_ST;
					break;
				case ISP_CAMSV4_IDX:
					LOG_INF("CAMSV_4 viewFinder is ON\n");
					cam_dmao =
						(ISP_RD32(CAMSV_REG_MODULE_EN(
						ISP_CAMSV4_IDX)) & 0x10);
					LOG_INF("CAMSV_4:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV4_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR,
							ISP_CAMSV4_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV4_IDX),
							(vf + 0x1));
					module = ISP_IRQ_TYPE_INT_CAMSV_4_ST;
					break;
				case ISP_CAMSV5_IDX:
					LOG_INF("CAMSV_5 viewFinder is ON\n");
					cam_dmao =
						(ISP_RD32(CAMSV_REG_MODULE_EN(
						ISP_CAMSV5_IDX)) & 0x10);
					LOG_INF("CAMSV_5:[DMA_EN]:0x%x\n",
						cam_dmao);
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV5_IDX));
					if (vf & 0x1) {
#define ERR_STR "module_%d: vf already enabled\n"
						LOG_ERR(ERR_STR,
							ISP_CAMSV5_IDX);
#undef ERR_STR
					} else
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV5_IDX),
							(vf + 0x1));
					module = ISP_IRQ_TYPE_INT_CAMSV_5_ST;
					break;
				default:
					LOG_ERR("unsupported module:0x%x\n",
						DebugFlag[1]);
					break;
				}
				pstRTBuf[module]->ring_buf[
					_camsv_imgo_].active =
					((cam_dmao & 0x10) ?
						(MTRUE) : (MFALSE));
				/*reset 1st sof flag when vf is enabled*/
				g1stSof[module] = MTRUE;
				break;
			}
			case 10: {
#define ERR_STR "module_%d: vf already disalbed\n"
				switch (DebugFlag[1]) {
				case ISP_CAMSV0_IDX:
					LOG_INF("CAMSV_0 viewFinder is OFF\n");
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV0_IDX));
					if (vf & 0x1)
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV0_IDX),
							(vf - 0x1));
					else
						LOG_ERR(ERR_STR,
							ISP_CAMSV0_IDX);
					break;
				case ISP_CAMSV1_IDX:
					LOG_INF("CAMSV_1 viewFinder is OFF\n");
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV1_IDX));
					if (vf & 0x1)
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV1_IDX),
							(vf - 0x1));
					else
						LOG_ERR(ERR_STR,
							ISP_CAMSV1_IDX);
					break;
				case ISP_CAMSV2_IDX:
					LOG_INF("CAMSV_2 viewFinder is OFF\n");
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV2_IDX));
					if (vf & 0x1)
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV2_IDX),
							(vf - 0x1));
					else
						LOG_ERR(ERR_STR,
							ISP_CAMSV2_IDX);
					break;
				case ISP_CAMSV3_IDX:
					LOG_INF("CAMSV_3 viewFinder is OFF\n");
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV3_IDX));
					if (vf & 0x1)
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV3_IDX),
							(vf - 0x1));
					else
						LOG_ERR(ERR_STR,
							ISP_CAMSV3_IDX);
					break;
				case ISP_CAMSV4_IDX:
					LOG_INF("CAMSV_4 viewFinder is OFF\n");
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV4_IDX));
					if (vf & 0x1)
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV4_IDX),
							(vf - 0x1));
					else
						LOG_ERR(ERR_STR,
							ISP_CAMSV4_IDX);
					break;
				case ISP_CAMSV5_IDX:
					LOG_INF("CAMSV_5 viewFinder is OFF\n");
					vf = ISP_RD32(CAMSV_REG_TG_VF_CON(
						ISP_CAMSV5_IDX));
					if (vf & 0x1)
						ISP_WR32(CAMSV_REG_TG_VF_CON(
							ISP_CAMSV5_IDX),
							(vf - 0x1));
					else
						LOG_ERR(ERR_STR,
							ISP_CAMSV5_IDX);
					break;
				default:
					LOG_ERR("unsupported module:0x%x\n",
						DebugFlag[1]);
					break;
				}
#undef ERR_STR
				break;
			}
			}
		} else {
			LOG_ERR("copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_GET_START_TIME:
		if (copy_from_user(DebugFlag, (void *)Param,
			sizeof(unsigned int) * 3) == 0) {
			struct S_START_T Tstp = {0,};

			#if (TIMESTAMP_QUEUE_EN == 1)
			unsigned int dma_id = DebugFlag[1];

			if (_cam_max_ == DebugFlag[1]) {
				/* only for wait timestamp to ready */
				Ret = ISP_WaitTimestampReady(DebugFlag[0],
					DebugFlag[2]);
				break;
			}

			switch (DebugFlag[0]) {
			case ISP_IRQ_TYPE_INT_CAM_A_ST:
			case ISP_IRQ_TYPE_INT_CAM_B_ST:
				if (ISP_PopBufTimestamp(DebugFlag[0], dma_id,
					&Tstp) != 0)
					LOG_ERR("Get Buf sof timestamp fail");

				break;
			case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
				Tstp.sec = gSTime[DebugFlag[0]].sec;
				Tstp.usec = gSTime[DebugFlag[0]].usec;
				break;
			default:
				LOG_ERR("unsupported module:0x%x\n",
					DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
			if (Ret != 0)
				break;
			#else
			if (DebugFlag[0] < ISP_IRQ_TYPE_INT_CAM_A_ST ||
				DebugFlag[0] > ISP_IRQ_TYPE_INT_UNI_A_ST) {
				Ret = -EFAULT;
				break;
			}

			if (g1stSof[DebugFlag[0]] == MFALSE) {
				Tstp.sec = gSTime[DebugFlag[0]].sec;
				Tstp.usec = gSTime[DebugFlag[0]].usec;
			} else
				Tstp.sec = Tstp.usec = 0;

			#endif

			switch (DebugFlag[0]) {
			case ISP_IRQ_TYPE_INT_CAM_A_ST:
			case ISP_IRQ_TYPE_INT_CAM_B_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
			case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
				if (copy_to_user((void *)Param, &Tstp,
					sizeof(struct S_START_T)) != 0) {
					LOG_ERR("copy_to_user failed");
					Ret = -EFAULT;
				}
				break;
			default:
				LOG_ERR("unsupported module:0x%x\n",
					DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
		}
		break;
	case ISP_DFS_CTRL:
	{
		static unsigned int camsys_qos;
		unsigned int dfs_ctrl;

		if (copy_from_user(&dfs_ctrl, (void *)Param,
			sizeof(unsigned int)) == 0) {
			if (dfs_ctrl == MTRUE) {
				if (++camsys_qos == 1) {
					mmdvfs_pm_qos_add_request(&isp_qos,
						MMDVFS_PM_QOS_SUB_SYS_CAMERA,
						0);
					LOG_VRB("CAMSYS PMQoS turn on");
				}
			} else {
				if (--camsys_qos == 0) {
					mmdvfs_pm_qos_remove_request(&isp_qos);
					LOG_VRB("CAMSYS PMQoS turn off");
				}
			}
		} else {
			LOG_ERR("ISP_DFS_CTRL copy_from_user failed\n");
			Ret = -EFAULT;
		}
	}
		break;
	case ISP_DFS_UPDATE:
	{
		unsigned int dfs_update;

		if (copy_from_user(&dfs_update, (void *)Param,
			sizeof(unsigned int)) == 0) {
			mmdvfs_pm_qos_update_request(&isp_qos,
				MMDVFS_PM_QOS_SUB_SYS_CAMERA, dfs_update);

			target_clk = dfs_update;
			LOG_VRB("Set clock level:%d", dfs_update);
		} else {
			LOG_ERR("ISP_DFS_UPDATE copy_from_user fail\n");
			Ret = -EFAULT;
		}
	}
		break;
	case ISP_GET_SUPPORTED_ISP_CLOCKS:
		/* To get how many clk levels this platform is supported */
		ispclks.clklevelcnt =
			mmdvfs_qos_get_thres_count(&isp_qos,
				MMDVFS_PM_QOS_SUB_SYS_CAMERA);

		if (ispclks.clklevelcnt > ISP_CLK_LEVEL_CNT)
			LOG_ERR("clklevelcnt is exceeded");

		for (; lv < ispclks.clklevelcnt; lv++) {
			/* To get all clk level on this platform */
			ispclks.clklevel[lv] =
				mmdvfs_qos_get_thres_value(&isp_qos,
					MMDVFS_PM_QOS_SUB_SYS_CAMERA, lv);

			LOG_VRB("DFS Clk level:%d", ispclks.clklevel[lv]);
		}

		if (copy_to_user((void *)Param, &ispclks,
			sizeof(struct ISP_CLK_INFO)) != 0) {
			LOG_ERR("copy_to_user failed");
			Ret = -EFAULT;
		}
		break;
	case ISP_GET_CUR_ISP_CLOCK:
		{
			struct ISP_GET_CLK_INFO getclk;

			getclk.curClk = mmdvfs_qos_get_cur_thres(&isp_qos,
				MMDVFS_PM_QOS_SUB_SYS_CAMERA);
			getclk.targetClk = target_clk;
			LOG_VRB("Get current clock level:%d, target clock:%d",
				getclk.curClk, getclk.targetClk);

			if (copy_to_user((void *)Param, &getclk,
				sizeof(struct ISP_GET_CLK_INFO)) != 0) {
				LOG_ERR("copy_to_user failed");
				Ret = -EFAULT;
			}
		}
		break;
	case ISP_GET_VSYNC_CNT:
		if (copy_from_user(&DebugFlag[0], (void *)Param,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("get cur sof from user fail");
			Ret = -EFAULT;
		} else {
			switch (DebugFlag[0]) {
			case ISP_IRQ_TYPE_INT_CAM_A_ST:
				DebugFlag[1] = Vsync_cnt[0];
				break;
			case ISP_IRQ_TYPE_INT_CAM_B_ST:
				DebugFlag[1] = Vsync_cnt[1];
				break;
			default:
				LOG_ERR("err TG(0x%x)\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
		}
		if (copy_to_user((void *)Param, &DebugFlag[1],
			sizeof(unsigned int)) != 0) {
			LOG_ERR("copy to user fail");
			Ret = -EFAULT;
		}
		break;
	case ISP_RESET_VSYNC_CNT:
		Vsync_cnt[0] = Vsync_cnt[1] = 0;
		break;
	#ifdef ENABLE_KEEP_ION_HANDLE
	case ISP_ION_IMPORT:
		if (copy_from_user(&IonNode, (void *)Param,
			sizeof(struct ISP_DEV_ION_NODE_STRUCT)) == 0) {
			struct T_ION_TBL *ptbl = NULL;
			unsigned int jump;

			if (!pIon_client) {
				LOG_ERR("ion_import: invalid ion client!\n");
				Ret = -EFAULT;
				break;
			}

			if (IonNode.devNode >= ISP_DEV_NODE_NUM) {
#define ERR_STR1 "[ISP_ION_IMPORT] devNode should be smaller"
#define ERR_STR2 " than ISP_DEV_NODE_NUM\n"
				LOG_ERR(ERR_STR1 ERR_STR2);
#undef ERR_STR1
#undef ERR_STR2
				Ret = -EFAULT;
				break;
			}

			ptbl = &gION_TBL[IonNode.devNode];
			if (ptbl->node != IonNode.devNode) {
				LOG_ERR("ion_import: devNode not support(%d)\n",
					IonNode.devNode);
				Ret = -EFAULT;
				break;
			}
			if (IonNode.dmaPort < 0 ||
				IonNode.dmaPort >= _dma_max_wr_) {
				LOG_ERR("ion_import: dmaport error:%d(0~%d)\n",
					IonNode.dmaPort, _dma_max_wr_);
				Ret = -EFAULT;
				break;
			}
			jump = IonNode.dmaPort * _ion_keep_max_;
			if (IonNode.memID <= 0) {
#define ERR_STR "ion_import: dma(%d)invalid ion fd(%d)\n"
				LOG_ERR(ERR_STR,
					IonNode.dmaPort, IonNode.memID);
#undef ERR_STR
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
				if (IspInfo.DebugMask & ISP_DBG_ION_CTRL) {
#define TMP_STR1 "ion_import: already exist: dev(%d), dma(%d),"
#define TMP_STR2 " i(%d), fd(%d), Hnd(0x%p)\n"
					LOG_INF(TMP_STR1 TMP_STR2,
						IonNode.devNode,
						IonNode.dmaPort, i,
						IonNode.memID,
						ptbl->pIonHnd[jump + i]);
#undef TMP_STR1
#undef TMP_STR2
				}
				/* User might allocate a big memory and divid it
				 * into many buffers, the ion FD of these
				 * buffers is the same, so we must check there
				 * has no users take this memory.
				 */
				ptbl->pIonCt[jump + i]++;

				break;
			}
			/* */
			handle = ISP_ion_import_handle(pIon_client,
				IonNode.memID);
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

					/* User might allocate a big memory and
					 * divid it into many buffers, the ion
					 * FD of these buffers is the same, so
					 * we must check there has no users take
					 * this memory.
					 */
					ptbl->pIonCt[jump + i]++;

					if (IspInfo.DebugMask &
						ISP_DBG_ION_CTRL) {
#define TMP_STR "ion_import: dev(%d), dma(%d), i(%d), fd(%d), Hnd(0x%p)\n"
						LOG_INF(TMP_STR,
							IonNode.devNode,
							IonNode.dmaPort, i,
							IonNode.memID, handle);
#undef TMP_STR
					}
					break;
				}
			}
			spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
			/* */
			if (i == _ion_keep_max_) {
#define ERR_STR "ion_import: dma(%d)no empty space in list(%d_%d)\n"
				LOG_ERR(ERR_STR,
					IonNode.dmaPort, IonNode.memID,
					_ion_keep_max_);
#undef ERR_STR
				ISP_ion_free_handle(pIon_client, handle);
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("[ion import]copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_ION_FREE:
		if (copy_from_user(&IonNode, (void *)Param,
			sizeof(struct ISP_DEV_ION_NODE_STRUCT)) == 0) {
			struct T_ION_TBL *ptbl = NULL;
			unsigned int jump;

			if (!pIon_client) {
				LOG_ERR("ion_free: invalid ion client!\n");
				Ret = -EFAULT;
				break;
			}

			if (IonNode.devNode >= ISP_DEV_NODE_NUM) {
#define ERR_STR "[ISP_ION_FREE]devNode should be smaller than ISP_DEV_NODE_NUM"
				LOG_ERR(ERR_STR);
#undef ERR_STR
				Ret = -EFAULT;
				break;
			}

			ptbl = &gION_TBL[IonNode.devNode];
			if (ptbl->node != IonNode.devNode) {
				LOG_ERR("ion_free: devNode not support(%d)!\n",
					IonNode.devNode);
				Ret = -EFAULT;
				break;
			}
			if (IonNode.dmaPort < 0 ||
				IonNode.dmaPort >= _dma_max_wr_) {
				LOG_ERR("ion_free: dmaport error:%d(0~%d)\n",
					IonNode.dmaPort, _dma_max_wr_);
				Ret = -EFAULT;
				break;
			}
			jump = IonNode.dmaPort*_ion_keep_max_;
			if (IonNode.memID <= 0) {
				LOG_ERR("ion_free: invalid ion fd(%d)\n",
					IonNode.memID);
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
#define ERR_STR "ion_free: can't find ion dev(%d), dma(%d), fd(%d) in list\n"
				LOG_ERR(ERR_STR,
					IonNode.devNode, IonNode.dmaPort,
					IonNode.memID);
#undef ERR_STR
				Ret = -EFAULT;

				break;
			}
			/* User might allocate a big memory and divid it into
			 * many buffers, the ion FD of these buffers is the
			 * same, so we must check there has no users take this
			 * memory.
			 */
			if (--ptbl->pIonCt[jump + i] > 0) {
				spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
				if (IspInfo.DebugMask & ISP_DBG_ION_CTRL) {
#define TMP_STR "ion_free: user ct(%d): dev(%d), dma(%d), i(%d), fd(%d)\n"
					LOG_INF(TMP_STR,
						ptbl->pIonCt[jump + i],
						IonNode.devNode,
						IonNode.dmaPort, i,
						IonNode.memID);
#undef TMP_STR
				}
				break;
			} else if (ptbl->pIonCt[jump + i] < 0) {
				spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
#define ERR_STR1 "ion_free: free more than import (%d): dev(%d),"
#define ERR_STR2 " dma(%d), i(%d), fd(%d)\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					ptbl->pIonCt[jump + i],
					IonNode.devNode, IonNode.dmaPort, i,
					IonNode.memID);
#undef ERR_STR1
#undef ERR_STR2
				Ret = -EFAULT;
				break;
			}

			if (IspInfo.DebugMask & ISP_DBG_ION_CTRL) {
#define TMP_STR "ion_free: dev(%d)dma(%d)i(%d)fd(%d)Hnd(0x%p)Ct(%d)\n"
				LOG_INF(TMP_STR,
					IonNode.devNode, IonNode.dmaPort, i,
					IonNode.memID,
					ptbl->pIonHnd[jump + i],
					ptbl->pIonCt[jump + i]);
#undef TMP_STR
			}

			p_IonHnd = ptbl->pIonHnd[jump + i];
			ptbl->pIonFd[jump + i] = 0;
			ptbl->pIonHnd[jump + i] = NULL;
			spin_unlock(&(ptbl->pLock[IonNode.dmaPort]));
			/* */
			ISP_ion_free_handle(pIon_client, p_IonHnd);
		} else {
			LOG_ERR("[ion free]copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	case ISP_ION_FREE_BY_HWMODULE:
		if (copy_from_user(&module, (void *)Param,
			sizeof(unsigned int)) == 0) {
			if (module >= ISP_DEV_NODE_NUM) {
#define ERR_STR1 "[ISP_ION_FREE_BY_HWMODULE]module should be smaller"
#define ERR_STR2 " than ISP_DEV_NODE_NUM"
				LOG_ERR(ERR_STR1 ERR_STR2);
#undef ERR_STR1
#undef ERR_STR2
				Ret = -EFAULT;
				break;
			}
			if (gION_TBL[module].node != module) {
				LOG_ERR("module error(%d)\n", module);
				Ret = -EFAULT;
				break;
			}

			ISP_ion_free_handle_by_module(module);
		} else {
			LOG_ERR("[ion free by module]copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	#endif
	case ISP_CQ_SW_PATCH: {
			static unsigned int Addr[2] = {0, 0};

			if (copy_from_user(DebugFlag, (void *)Param,
				sizeof(unsigned int)*2) == 0) {
				switch (DebugFlag[0]) {
				case ISP_IRQ_TYPE_INT_CAM_A_ST:
					Addr[0] = DebugFlag[1];
					break;
				case ISP_IRQ_TYPE_INT_CAM_B_ST:
					Addr[1] = DebugFlag[1];
					break;
				default:
					LOG_ERR("unsupported module:0x%x\n",
						DebugFlag[0]);
					break;
				}
			}
			if ((Addr[0] != 0) && (Addr[1] != 0)) {
				unsigned long flags;

				spin_lock_irqsave(&IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAM_A_ST], flags);
				ISP_WR32(CAM_REG_CTL_CD_DONE_SEL(
					ISP_CAM_A_IDX), 0x1);
				ISP_WR32(CAM_REG_CTL_UNI_DONE_SEL(
					ISP_CAM_A_IDX), 0x1);
				ISP_WR32(CAM_REG_CQ_THR0_BASEADDR(
					ISP_CAM_B_IDX), Addr[1]);
				ISP_WR32(CAM_REG_CQ_THR0_BASEADDR(
					ISP_CAM_A_IDX), Addr[0]);
				ISP_WR32(CAM_REG_CTL_UNI_DONE_SEL(
					ISP_CAM_A_IDX), 0x0);
				Addr[0] = Addr[1] = 0;
				spin_unlock_irqrestore(&IspInfo.SpinLockIrq[
					ISP_IRQ_TYPE_INT_CAM_A_ST], flags);
			}
		}
		break;
	#if (SMI_LARB_MMU_CTL == 1)
	case ISP_LARB_MMU_CTL: {

		struct ISP_LARB_MMU_STRUCT larbInfo;

		if (copy_from_user(&larbInfo, (void *)Param,
			sizeof(struct ISP_LARB_MMU_STRUCT)) != 0) {
			LOG_ERR("copy_from_user LARB_MMU_CTL failed\n");
			Ret = -EFAULT;
			goto EXIT;
		}

		switch (larbInfo.LarbNum) {
		case 2:
		case 3:
		case 5:
		case 6:
			break;
		default:
			LOG_ERR("Wrong SMI_LARB port=%d\n", larbInfo.LarbNum);
			Ret = -EFAULT;
			goto EXIT;
		}

		if ((SMI_LARB_BASE[larbInfo.LarbNum] == NULL) ||
			(larbInfo.regOffset >= 0x1000)) {
#define ERR_STR "Wrong SMI_LARB port=%d, base addr=%p, offset=0x%x\n"
			LOG_ERR(ERR_STR, larbInfo.LarbNum,
				SMI_LARB_BASE[larbInfo.LarbNum],
				larbInfo.regOffset);
#undef ERR_STR
			Ret = -EFAULT;
			goto EXIT;
		}

		*(unsigned int *)((char *)SMI_LARB_BASE[larbInfo.LarbNum] +
			larbInfo.regOffset) = larbInfo.regVal;

		}
		break;
	#endif
	case ISP_GET_DUMP_INFO: {
		if (copy_to_user((void *)Param, &g_dumpInfo,
			sizeof(struct ISP_GET_DUMP_INFO_STRUCT)) != 0) {
			LOG_ERR("ISP_GET_DUMP_INFO copy to user fail");
			Ret = -EFAULT;
		}
		break;
	}
	case ISP_DUMP_BUFFER: {
		if (copy_from_user(&DumpBufStruct, (void *)Param,
			sizeof(struct ISP_DUMP_BUFFER_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented
			 * in ISP_DumpTuningBuffer(...)
			 */
			Ret = ISP_DumpBuffer(&DumpBufStruct);
		} else {
			LOG_ERR("ISP_DUMP_TUNING_BUFFER copy_from_user fail\n");
			Ret = -EFAULT;
		}
		break;
	}
	case ISP_SET_MEM_INFO: {
		if (copy_from_user(&MemInfoStruct, (void *)Param,
			sizeof(struct ISP_MEM_INFO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented
			 * in ISP_SetMemInfo(...)
			 */
			Ret = ISP_SetMemInfo(&MemInfoStruct);
		} else {
			LOG_ERR("ISP_SET_MEM_INFO copy_from_user failed\n");
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
	if (Ret != 0) {
#define ERR_STR "Fail, Cmd(%d), Pid(%d), (process, pid, tgid)=(%s, %d, %d)\n"
		LOG_ERR(ERR_STR,
			Cmd, pUserInfo->Pid,
			current->comm, current->pid, current->tgid);
#undef ERR_STR
	}
	/*  */
	return Ret;
}

#ifdef CONFIG_COMPAT

/*******************************************************************************
 *
 ******************************************************************************/
static int compat_get_isp_read_register_data(
	struct compat_ISP_REG_IO_STRUCT __user *data32,
	struct ISP_REG_IO_STRUCT __user *data)
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

static int compat_put_isp_read_register_data(
	struct compat_ISP_REG_IO_STRUCT __user *data32,
	struct ISP_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	int err = 0;

	err |= get_user(count, &data->Count);
	err |= put_user(count, &data32->Count);
	return err;
}





static int compat_get_isp_buf_ctrl_struct_data(
	struct compat_ISP_BUFFER_CTRL_STRUCT __user *data32,
	struct ISP_BUFFER_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp, tmp2, tmp3;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(tmp, &data32->ctrl);
	err |= put_user(tmp, &data->ctrl);
	err |= get_user(tmp2, &data32->module);
	err |= put_user(tmp2, &data->module);
	err |= get_user(tmp3, &data32->buf_id);
	err |= put_user(tmp3, &data->buf_id);
	err |= get_user(uptr, &data32->data_ptr);
	err |= put_user(compat_ptr(uptr), &data->data_ptr);
	err |= get_user(uptr, &data32->ex_data_ptr);
	err |= put_user(compat_ptr(uptr), &data->ex_data_ptr);
	err |= get_user(uptr, &data32->pExtend);
	err |= put_user(compat_ptr(uptr), &data->pExtend);

	return err;
}

static int compat_put_isp_buf_ctrl_struct_data(
	struct compat_ISP_BUFFER_CTRL_STRUCT __user *data32,
	struct ISP_BUFFER_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp, tmp2, tmp3;
	int err = 0;

	err = get_user(tmp, &data->ctrl);
	err |= put_user(tmp, &data32->ctrl);
	err |= get_user(tmp2, &data->module);
	err |= put_user(tmp2, &data32->module);
	err |= get_user(tmp3, &data->buf_id);
	err |= put_user(tmp3, &data32->buf_id);

	return err;
}

static int compat_get_isp_ref_cnt_ctrl_struct_data(
	struct compat_ISP_REF_CNT_CTRL_STRUCT __user *data32,
	struct ISP_REF_CNT_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp, tmp2;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(tmp, &data32->ctrl);
	err |= put_user(tmp, &data->ctrl);
	err |= get_user(tmp2, &data32->id);
	err |= put_user(tmp2, &data->id);
	err |= get_user(uptr, &data32->data_ptr);
	err |= put_user(compat_ptr(uptr), &data->data_ptr);

	return err;
}

static int compat_put_isp_ref_cnt_ctrl_struct_data(
	struct compat_ISP_REF_CNT_CTRL_STRUCT __user *data32,
	struct ISP_REF_CNT_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp, tmp2;
	int err = 0;

	err = get_user(tmp, &data->ctrl);
	err |= put_user(tmp, &data32->ctrl);
	err |= get_user(tmp2, &data->id);
	err |= put_user(tmp2, &data32->id);

	return err;
}

static int compat_get_isp_dump_buffer(
	struct compat_ISP_DUMP_BUFFER_STRUCT __user *data32,
	struct ISP_DUMP_BUFFER_STRUCT __user *data)
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

static int compat_get_isp_mem_info(
	struct compat_ISP_MEM_INFO_STRUCT __user *data32,
	struct ISP_MEM_INFO_STRUCT __user *data)
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

static long ISP_ioctl_compat(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	long ret = 0;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_ISP_READ_REGISTER: {
		struct compat_ISP_REG_IO_STRUCT __user *data32;
		struct ISP_REG_IO_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_isp_read_register_data(data32, data);
		if (err) {
			LOG_INF("compat_get_isp_read_register_data error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, ISP_READ_REGISTER,
			(unsigned long)data);
		err = compat_put_isp_read_register_data(data32, data);
		if (err) {
			LOG_INF("compat_put_isp_read_register_data error!!!\n");
			return err;
		}
		return ret;
	}
	case COMPAT_ISP_WRITE_REGISTER: {
		struct compat_ISP_REG_IO_STRUCT __user *data32;
		struct ISP_REG_IO_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_isp_read_register_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_ISP_WRITE_REGISTER error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, ISP_WRITE_REGISTER,
			(unsigned long)data);
		return ret;
	}
	case COMPAT_ISP_BUFFER_CTRL: {
		struct compat_ISP_BUFFER_CTRL_STRUCT __user *data32;
		struct ISP_BUFFER_CTRL_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_isp_buf_ctrl_struct_data(data32, data);
		if (err)
			return err;

		if (err) {
			LOG_INF("compat_get_isp_buf_ctrl_struct_data error!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, ISP_BUFFER_CTRL,
			(unsigned long)data);
		err = compat_put_isp_buf_ctrl_struct_data(data32, data);

		if (err) {
			LOG_INF("compat_put_isp_buf_ctrl_struct_data error!\n");
			return err;
		}
		return ret;

	}
	case COMPAT_ISP_REF_CNT_CTRL: {
		struct compat_ISP_REF_CNT_CTRL_STRUCT __user *data32;
		struct ISP_REF_CNT_CTRL_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_isp_ref_cnt_ctrl_struct_data(data32, data);
		if (err) {
#define ERR_STR "compat_get_isp_ref_cnt_ctrl_struct_data error!!!\n"
			LOG_INF(ERR_STR);
#undef ERR_STR
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, ISP_REF_CNT_CTRL,
			(unsigned long)data);
		err = compat_put_isp_ref_cnt_ctrl_struct_data(data32, data);
		if (err) {
#define ERR_STR "compat_put_isp_ref_cnt_ctrl_struct_data error!!!\n"
			LOG_INF(ERR_STR);
#undef ERR_STR
			return err;
		}
		return ret;
	}
	case COMPAT_ISP_DEBUG_FLAG: {
		/* compat_ptr(arg) will convert the arg */
		ret = filp->f_op->unlocked_ioctl(filp, ISP_DEBUG_FLAG,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_GET_INT_ERR: {
		/* compat_ptr(arg) will convert the arg */
		ret = filp->f_op->unlocked_ioctl(filp, ISP_GET_INT_ERR,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_GET_DMA_ERR: {
		/* compat_ptr(arg) will convert the arg */
		ret = filp->f_op->unlocked_ioctl(filp, ISP_GET_DMA_ERR,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_WAKELOCK_CTRL: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_WAKELOCK_CTRL,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_GET_DROP_FRAME: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_GET_DROP_FRAME,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_GET_CUR_SOF: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_GET_CUR_SOF,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_RESET_BY_HWMODULE: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_RESET_BY_HWMODULE,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_DUMP_ISR_LOG: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_DUMP_ISR_LOG,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_VF_LOG: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_VF_LOG,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_GET_START_TIME: {
		ret = filp->f_op->unlocked_ioctl(filp, ISP_GET_START_TIME,
			(unsigned long)compat_ptr(arg));
		return ret;
	}
	case COMPAT_ISP_DUMP_BUFFER: {
		struct compat_ISP_DUMP_BUFFER_STRUCT __user *data32;
		struct ISP_DUMP_BUFFER_STRUCT __user *data;

		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_isp_dump_buffer(data32, data);
		if (err) {
			LOG_INF("COMPAT_ISP_DUMP_BUFFER error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, ISP_DUMP_BUFFER,
			(unsigned long)data);
		return ret;
	}
	case COMPAT_ISP_SET_MEM_INFO: {
		struct compat_ISP_MEM_INFO_STRUCT __user *data32;
		struct ISP_MEM_INFO_STRUCT __user *data;
		int err = 0;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;
		err = compat_get_isp_mem_info(data32, data);
		if (err) {
			LOG_INF("COMPAT_ISP_SET_MEM_INFO error!!!\n");
			return err;
		}
		ret = filp->f_op->unlocked_ioctl(filp, ISP_SET_MEM_INFO,
			(unsigned long)data);
		return ret;
	}
	case ISP_GET_DUMP_INFO:
	case ISP_WAIT_IRQ:
	case ISP_CLEAR_IRQ: /* structure (no pointer) */
	case ISP_REGISTER_IRQ_USER_KEY:
	case ISP_MARK_IRQ_REQUEST:
	case ISP_GET_MARK2QUERY_TIME:
	case ISP_FLUSH_IRQ_REQUEST:
	case ISP_P2_BUFQUE_CTRL:/* structure (no pointer) */
	case ISP_UPDATE_REGSCEN:
	case ISP_QUERY_REGSCEN:
	case ISP_UPDATE_BURSTQNUM:
	case ISP_QUERY_BURSTQNUM:
	case ISP_DUMP_REG:
	case ISP_GET_VSYNC_CNT:
	case ISP_RESET_VSYNC_CNT:
	case ISP_ION_IMPORT:
	case ISP_ION_FREE:
	case ISP_ION_FREE_BY_HWMODULE:
	case ISP_CQ_SW_PATCH:
	case ISP_LARB_MMU_CTL:
	case ISP_DFS_CTRL:
	case ISP_DFS_UPDATE:
	case ISP_GET_SUPPORTED_ISP_CLOCKS:
	case ISP_GET_CUR_ISP_CLOCK:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return ISP_ioctl(filep, cmd, arg); */
	}
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_open(
	struct inode *pInode,
	struct file *pFile)
{
	int Ret = 0;
	unsigned int i, j;
	int q = 0, p = 0;
	struct ISP_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("- E. UserCount: %d.\n", IspInfo.UserCount);

	mutex_lock(&gDipMutex);  /* Protect the Multi Process */
	/*  */
	spin_lock(&(IspInfo.SpinLockIspRef));

	pFile->private_data = NULL;
	pFile->private_data =
		kmalloc(sizeof(struct ISP_USER_INFO_STRUCT), GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc fail, (proc, pid, tgid)=(%s, %d, %d)\n",
			current->comm, current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (struct ISP_USER_INFO_STRUCT *)pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
#define TMP_STR "Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d),"
	if (IspInfo.UserCount > 0) {
		IspInfo.UserCount++;
		spin_unlock(&(IspInfo.SpinLockIspRef));
		LOG_DBG(TMP_STR " users exist\n",
			IspInfo.UserCount, current->comm, current->pid,
			current->tgid);
		goto EXIT;
	} else {
		IspInfo.UserCount++;
		spin_unlock(&(IspInfo.SpinLockIspRef));

		/* kernel log limit to (current+150) lines per second */
	#if (_K_LOG_ADJUST == 1)
		pr_detect_count = get_detect_count();
		i = pr_detect_count + 150;
		set_detect_count(i);

		LOG_DBG(TMP_STR " log_limit_line(%d), first user\n",
			IspInfo.UserCount, current->comm, current->pid,
			current->tgid, i);
	#else
		LOG_DBG(TMP_STR " first user\n",
			IspInfo.UserCount, current->comm, current->pid,
			current->tgid);
	#endif
	}
#undef TMP_STR
	/* do wait queue head init when re-enter in camera */
	/*  */
	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		FirstUnusedIrqUserKey = 1;
		strncpy((void *)IrqUserKey_UserInfo[i].userName,
			"DefaultUserNametoAllocMem", USERKEY_STR_LEN);
		IrqUserKey_UserInfo[i].userKey = -1;
	}
	/*  */
	spin_lock(&(SpinLock_P2FrameList));
	for (q = 0; q < ISP_P2_BUFQUE_PROPERTY_NUM; q++) {
		for (i = 0; i < _MAX_SUPPORT_P2_FRAME_NUM_; i++) {
			P2_FrameUnit_List[q][i].processID = 0x0;
			P2_FrameUnit_List[q][i].callerID = 0x0;
			P2_FrameUnit_List[q][i].cqMask =  0x0;
			P2_FrameUnit_List[q][i].bufSts = ISP_P2_BUF_STATE_NONE;
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
	IspInfo.BufInfo.Read.pData = kmalloc(ISP_BUF_SIZE, GFP_ATOMIC);
	IspInfo.BufInfo.Read.Size = ISP_BUF_SIZE;
	IspInfo.BufInfo.Read.Status = ISP_BUF_STATUS_EMPTY;
	if (IspInfo.BufInfo.Read.pData == NULL) {
		LOG_DBG("ERROR: BufRead kmalloc failed\n");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/*mutex_lock(&gDipMutex);*/  /* Protect the Multi Process */
	g_bIonBufferAllocated = MFALSE;
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
	g_isp_p2_imem_buf.handle = NULL;
	g_isp_p2_imem_buf.ion_fd = 0;
	g_isp_p2_imem_buf.va = 0;
	g_isp_p2_imem_buf.pa = 0;
	g_isp_p2_imem_buf.length =
		((4 * ISP_DIP_REG_SIZE) + (2 * MAX_ISP_TILE_TDR_HEX_NO) +
		(2 * MAX_ISP_CMDQ_BUFFER_SIZE) + (8 * 0x400));
	isp_p2_ion_client = NULL;
	if ((isp_p2_ion_client == NULL) && (g_ion_device))
		isp_p2_ion_client = ion_client_create(g_ion_device, "isp_p2");
	if (isp_p2_ion_client == NULL)
		LOG_ERR("invalid isp_p2_ion_client client!\n");
	if (isp_allocbuf(&g_isp_p2_imem_buf) >= 0)
		g_bIonBufferAllocated = MTRUE;
#endif

	if (g_bIonBufferAllocated == MTRUE) {
		g_pPhyISPBuffer = (unsigned int *)
			(uintptr_t)(g_isp_p2_imem_buf.va);
		g_pTuningBuffer = (unsigned int *)
			(((uintptr_t)g_pPhyISPBuffer) + ISP_DIP_REG_SIZE);
		g_pTpipeBuffer = (unsigned int *)
			(((uintptr_t)g_pTuningBuffer) + ISP_DIP_REG_SIZE);
		g_pVirISPBuffer = (unsigned int *)
			(((uintptr_t)g_pTpipeBuffer) + MAX_ISP_TILE_TDR_HEX_NO);
		g_pCmdqBuffer = (unsigned int *)
			(((uintptr_t)g_pVirISPBuffer) + ISP_DIP_REG_SIZE);
		/* Kernel Exception */
		g_pKWTpipeBuffer = (unsigned int *)
			(((uintptr_t)g_pCmdqBuffer) + MAX_ISP_CMDQ_BUFFER_SIZE);
		g_pKWCmdqBuffer = (unsigned int *)
			(((uintptr_t)g_pKWTpipeBuffer) +
			MAX_ISP_TILE_TDR_HEX_NO);
		g_pKWVirISPBuffer = (unsigned int *)
			(((uintptr_t)g_pKWCmdqBuffer) +
			MAX_ISP_CMDQ_BUFFER_SIZE);
	} else {
		/* Navtive Exception */
		g_pPhyISPBuffer = NULL;
		g_pTuningBuffer = NULL;
		g_pTpipeBuffer = NULL;
		g_pVirISPBuffer = NULL;
		g_pCmdqBuffer = NULL;
		/* Kernel Exception */
		g_pKWTpipeBuffer = NULL;
		g_pKWCmdqBuffer = NULL;
		g_pKWVirISPBuffer = NULL;
	}
	g_bUserBufIsReady = MFALSE;
	g_bDumpPhyISPBuf = MFALSE;
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
	/*mutex_unlock(&gDipMutex);*/  /* Protect the Multi Process */

	/*  */
	for (i = 0; i < ISP_REF_CNT_ID_MAX; i++)
		atomic_set(&g_imem_ref_cnt[i], 0);

	/*  */
	for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
		for (j = 0; j < ISP_IRQ_ST_AMOUNT; j++) {
			for (q = 0; q < IRQ_USER_NUM_MAX; q++) {
				IspInfo.IrqInfo.Status[i][j][q] = 0;
				IspInfo.IrqInfo.MarkedFlag[i][j][q] = 0;
				for (p = 0; p < 32; p++) {
					IspInfo.IrqInfo.
						MarkedTime_sec[i][p][q] = 0;
					IspInfo.IrqInfo.
						MarkedTime_usec[i][p][q] = 0;
					IspInfo.IrqInfo.
						PassedBySigCnt[i][p][q] = 0;
					IspInfo.IrqInfo.
						LastestSigTime_sec[i][p] = 0;
					IspInfo.IrqInfo.
						LastestSigTime_usec[i][p] = 0;
				}
			}
		}
	}
	/* reset backup regs*/
	memset(g_BkReg, 0, sizeof(struct _isp_bk_reg_t) * ISP_IRQ_TYPE_AMOUNT);

#ifdef ENABLE_KEEP_ION_HANDLE
	/* create ion client*/
	ISP_ion_init();
#endif

#ifdef KERNEL_LOG
	IspInfo.DebugMask = (ISP_DBG_INT);
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
		ISP_EnableClock(MTRUE);
	}

	mutex_unlock(&gDipMutex);  /* Protect the Multi Process */
	LOG_INF("- X. Ret: %d. UserCount: %d. G_u4EnableClockCount:%d\n", Ret,
		IspInfo.UserCount, G_u4EnableClockCount);
	return Ret;

}

/*******************************************************************************
 *
 ******************************************************************************/
static inline void ISP_StopHW(int module)
{
	unsigned int regTGSt, loopCnt;
	int ret = 0;
	struct ISP_WAIT_IRQ_STRUCT waitirq;
	ktime_t             time;
	unsigned long long  sec = 0, m_sec = 0;
	unsigned long long  timeoutMs = 500000000;/*500ms*/
	char moduleName[128];

	if (module == ISP_CAM_A_IDX)
		strncpy(moduleName, "CAMA", 5);
	else
		strncpy(moduleName, "CAMB", 5);

	/* wait TG idle*/
	loopCnt = 3;
	waitirq.Type = (module == ISP_CAM_A_IDX) ?
		ISP_IRQ_TYPE_INT_CAM_A_ST : ISP_IRQ_TYPE_INT_CAM_B_ST;
	waitirq.EventInfo.Clear = ISP_IRQ_CLEAR_WAIT;
	waitirq.EventInfo.Status = VS_INT_ST;
	waitirq.EventInfo.St_type = SIGNAL_INT;
	waitirq.EventInfo.Timeout = 0x100;
	waitirq.EventInfo.UserKey = 0x0;
	waitirq.bDumpReg = 0;

	do {
		regTGSt = (ISP_RD32(CAM_REG_TG_INTER_ST(module)) & 0x00003F00)
			>> 8;
		if (regTGSt == 1)
			break;

		LOG_INF("%s: wait 1VD (%d)\n", moduleName, loopCnt);
		ret = ISP_WaitIrq(&waitirq);
		/* first wait is clear wait, others are non-clear wait */
		waitirq.EventInfo.Clear = ISP_IRQ_CLEAR_NONE;
	} while (--loopCnt);

	if (-ERESTARTSYS == ret) {
		LOG_INF("%s: interrupt by system signal, wait idle\n",
			moduleName);
		/* timer*/
		time = ktime_get();
		m_sec = time.tv64;

		while (regTGSt != 1) {
			regTGSt = (ISP_RD32(CAM_REG_TG_INTER_ST(module)) &
				0x00003F00) >> 8;
			/*timer*/
			time = ktime_get();
			sec = time.tv64;
			/* wait time>timeoutMs, break */
			if ((sec - m_sec) > timeoutMs)
				break;
		}
		if (regTGSt == 1)
			LOG_INF("%s: wait idle done\n", moduleName);
		else
			LOG_INF("%s: wait idle timeout(%lld)\n", moduleName,
				(sec - m_sec));
	}

	if (-EFAULT == ret || regTGSt != 1) {
		LOG_INF("%s: reset\n", moduleName);
		/* timer*/
		time = ktime_get();
		m_sec = time.tv64;

		/* Reset*/
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x1);
		while (ISP_RD32(CAM_REG_CTL_SW_CTL(module)) != 0x2) {
			/*LOG_DBG("%s resetting...\n", moduleName);*/
			/*timer*/
			time = ktime_get();
			sec = time.tv64;
			/* wait time>timeoutMs, break */
			if ((sec  - m_sec) > timeoutMs) {
				LOG_INF("%s: wait SW idle timeout\n",
					moduleName);
				break;
			}
		}
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x4);
		ISP_WR32(CAM_REG_CTL_SW_CTL(module), 0x0);
		regTGSt = (ISP_RD32(CAM_REG_TG_INTER_ST(module)) & 0x00003F00)
			>> 8;
		LOG_DBG("%s_TG_ST(%d)_SW_ST(0x%x)\n", moduleName, regTGSt,
			ISP_RD32(CAM_REG_CTL_SW_CTL(module)));
	}

	/*disable CMOS*/
	ISP_WR32(CAM_REG_TG_SEN_MODE(module),
		(ISP_RD32(CAM_REG_TG_SEN_MODE(module))&0xfffffffe));

}

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_release(
	struct inode *pInode,
	struct file *pFile)
{
	struct ISP_USER_INFO_STRUCT *pUserInfo;
	unsigned int Reg;
	unsigned int i = 0;

	LOG_DBG("- E. UserCount: %d.\n", IspInfo.UserCount);

	/*  */

	/*  */
	/* LOG_DBG("UserCount(%d)",IspInfo.UserCount); */
	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo = (struct ISP_USER_INFO_STRUCT *)pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	mutex_lock(&gDipMutex);  /* Protect the Multi Process */
	/*      */
#define TMP_STR "Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), "
	spin_lock(&(IspInfo.SpinLockIspRef));
	IspInfo.UserCount--;
	if (IspInfo.UserCount > 0) {
		spin_unlock(&(IspInfo.SpinLockIspRef));
		LOG_DBG(TMP_STR "users exist",
			IspInfo.UserCount, current->comm, current->pid,
			current->tgid);
		goto EXIT;
	} else {
		spin_unlock(&(IspInfo.SpinLockIspRef));
	}

	/* kernel log limit back to default */
#if (_K_LOG_ADJUST == 1)
	set_detect_count(pr_detect_count);
#endif
	/*      */
	LOG_DBG(TMP_STR "log_limit_line(%d), last user",
		IspInfo.UserCount, current->comm, current->pid, current->tgid,
		pr_detect_count);
#undef TMP_STR
	/* Close VF when ISP_release */
	/* reason of close vf is to make sure camera can serve regular after
	 * previous abnormal exit.
	 */
	Reg = ISP_RD32(CAM_REG_TG_VF_CON(ISP_CAM_A_IDX));
	Reg &= 0xfffffffE;/* close Vfinder */
	ISP_WR32(CAM_REG_TG_VF_CON(ISP_CAM_A_IDX), Reg);

	Reg = ISP_RD32(CAM_REG_TG_VF_CON(ISP_CAM_B_IDX));
	Reg &= 0xfffffffE;/* close Vfinder */
	ISP_WR32(CAM_REG_TG_VF_CON(ISP_CAM_B_IDX), Reg);

	for (i = ISP_CAMSV0_IDX; i <= ISP_CAMSV3_IDX; i++) {
		Reg = ISP_RD32(CAM_REG_TG_VF_CON(i));
		Reg &= 0xfffffffE;/* close Vfinder */
		ISP_WR32(CAM_REG_TG_VF_CON(i), Reg);
	}

	/* Set DMX_SEL = 0 when ISP_release */
	/* Reson: If twin is enabled, the twin module's DMX_SEL will be set
	 * to 1. It will encounter err when run single path and other module
	 * dmx_sel = 1.
	 */
	Reg = ISP_RD32(CAM_REG_CTL_SEL(ISP_CAM_A_IDX));
	Reg &= 0xfffffff8;/* set dmx to 0 */
	ISP_WR32(CAM_REG_CTL_SEL(ISP_CAM_A_IDX), Reg);

	Reg = ISP_RD32(CAM_REG_CTL_SEL(ISP_CAM_B_IDX));
	Reg &= 0xfffffff8;/* set dmx to 0 */
	ISP_WR32(CAM_REG_CTL_SEL(ISP_CAM_B_IDX), Reg);

	/* Reset Twin status.
	 *  If previous camera run in twin mode,
	 *  then mediaserver died, no one clear this status.
	 *  Next camera runs in single mode, and it will not update CQ0
	 */
	ISP_WR32(CAM_REG_CTL_TWIN_STATUS(ISP_CAM_A_IDX), 0x0);
	ISP_WR32(CAM_REG_CTL_TWIN_STATUS(ISP_CAM_B_IDX), 0x0);

	/* why i add this wake_unlock here, because the Ap is not expected to
	 * be dead. The driver must releae the wakelock, otherwise the system
	 * will not enter the power-saving mode
	 */
	if (g_WaitLockCt) {
		LOG_INF("wakelock disable!! cnt(%d)\n", g_WaitLockCt);
#ifdef CONFIG_PM_WAKELOCKS
		__pm_relax(&isp_wake_lock);
#else
		wake_unlock(&isp_wake_lock);
#endif
		g_WaitLockCt = 0;
	}
	/* reset */
	/*      */
	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		FirstUnusedIrqUserKey = 1;
		strncpy((void *)IrqUserKey_UserInfo[i].userName,
			"DefaultUserNametoAllocMem", USERKEY_STR_LEN);
		IrqUserKey_UserInfo[i].userKey = -1;
	}
	if (IspInfo.BufInfo.Read.pData != NULL) {
		kfree(IspInfo.BufInfo.Read.pData);
		IspInfo.BufInfo.Read.pData = NULL;
		IspInfo.BufInfo.Read.Size = 0;
		IspInfo.BufInfo.Read.Status = ISP_BUF_STATUS_EMPTY;
	}
	/*mutex_lock(&gDipMutex);*/  /* Protect the Multi Process */
	if (g_bIonBufferAllocated == MFALSE) {
		/* Native Exception */
		if (g_pPhyISPBuffer != NULL) {
			vfree(g_pPhyISPBuffer);
			g_pPhyISPBuffer = NULL;
		}
		if (g_pTuningBuffer != NULL) {
			vfree(g_pTuningBuffer);
			g_pTuningBuffer = NULL;
		}
		if (g_pTpipeBuffer != NULL) {
			vfree(g_pTpipeBuffer);
			g_pTpipeBuffer = NULL;
		}
		if (g_pVirISPBuffer != NULL) {
			vfree(g_pVirISPBuffer);
			g_pVirISPBuffer = NULL;
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
		if (g_pKWVirISPBuffer != NULL) {
			vfree(g_pKWVirISPBuffer);
			g_pKWVirISPBuffer = NULL;
		}
	} else {
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
		if (g_isp_p2_imem_buf.handle != NULL) {
			isp_freebuf(&g_isp_p2_imem_buf);
			g_isp_p2_imem_buf.handle = NULL;
			g_isp_p2_imem_buf.ion_fd = 0;
			g_isp_p2_imem_buf.va = 0;
			g_isp_p2_imem_buf.pa = 0;
		}
		g_bIonBufferAllocated = MFALSE;
		/* Navtive Exception */
		g_pPhyISPBuffer = NULL;
		g_pTuningBuffer = NULL;
		g_pTpipeBuffer = NULL;
		g_pVirISPBuffer = NULL;
		g_pCmdqBuffer = NULL;
		/* Kernel Exception */
		g_pKWTpipeBuffer = NULL;
		g_pKWCmdqBuffer = NULL;
		g_pKWVirISPBuffer = NULL;
#endif
	}
	/*mutex_unlock(&gDipMutex);*/

#ifdef AEE_DUMP_BY_USING_ION_MEMORY
	if (isp_p2_ion_client != NULL) {
		ion_client_destroy(isp_p2_ion_client);
		isp_p2_ion_client = NULL;
	} else {
		LOG_ERR("isp_p2_ion_client is NULL!!\n");
	}
#endif
	/* reset backup regs*/
	memset(g_BkReg, 0, sizeof(struct _isp_bk_reg_t) * ISP_IRQ_TYPE_AMOUNT);

	/*  */
#ifdef ENABLE_KEEP_ION_HANDLE
	ISP_StopHW(ISP_CAM_A_IDX);
	ISP_StopHW(ISP_CAM_B_IDX);

	/* free keep ion handles, then destroy ion client*/
	for (i = 0; i < ISP_DEV_NODE_NUM; i++) {
		if (gION_TBL[i].node != ISP_DEV_NODE_NUM)
			ISP_ion_free_handle_by_module(i);
	}

	ISP_ion_uninit();
#endif

	/*  */
	/* LOG_DBG("Before spm_enable_sodi()."); */
	/* Enable sodi (Multi-Core Deep Idle). */

#if 0 /* _mt6593fpga_dvt_use_ */
	spm_enable_sodi();
#endif

	/* Disable clock.
	 *  1. clkmgr: G_u4EnableClockCount=0, call clk_enable/disable
	 *  2. CCF: call clk_enable/disable every time
	 *     -> when IspInfo.UserCount, disable all ISP clk
	 */
	spin_lock(&(IspInfo.SpinLockClock));
	i = G_u4EnableClockCount;
	spin_unlock(&(IspInfo.SpinLockClock));
	while (i > 0) {
	ISP_EnableClock(MFALSE);
		i--;
	}

EXIT:
	mutex_unlock(&gDipMutex);
	LOG_INF("- X. UserCount: %d. G_u4EnableClockCount:%d",
		IspInfo.UserCount,
		G_u4EnableClockCount);
	return 0;
}


/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	unsigned long length = 0;
	unsigned int pfn = 0x0;

	/*LOG_DBG("- E.");*/
	length = (pVma->vm_end - pVma->vm_start);
	/*  */
	pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
	pfn = pVma->vm_pgoff << PAGE_SHIFT;
#if 0
#define TMP_STR1 "%s: vm_pgoff(0x%lx), pfn(0x%x), phy(0x%lx),"
#define TMP_STR2 " vm_start(0x%lx), vm_end(0x%lx), length(0x%lx)\n"
	LOG_INF(TMP_STR1 TMP_STR2, __func__, pVma->vm_pgoff, pfn,
		pVma->vm_pgoff << PAGE_SHIFT, pVma->vm_start, pVma->vm_end,
		length);
#undef TMP_STR1
#undef TMP_STR2
#endif

#define TMP_STR "mmap range error :module(0x%x), length(0x%lx), "
	switch (pfn) {
	case CAM_A_BASE_HW:
	case CAM_B_BASE_HW:
	case CAMSV_0_BASE_HW:
	case CAMSV_1_BASE_HW:
	case CAMSV_2_BASE_HW:
	case CAMSV_3_BASE_HW:
	case CAMSV_4_BASE_HW:
	case CAMSV_5_BASE_HW:
	case UNI_A_BASE_HW:
		if (length > ISP_REG_RANGE) {
			LOG_ERR(TMP_STR "ISP_REG_RANGE(0x%lx)!\n",
				pfn, length, ISP_REG_RANGE);
			return -EAGAIN;
		}
		break;
	case DIP_A_BASE_HW:
		if (length > ISP_REG_PER_DIP_RANGE) {
			LOG_ERR(TMP_STR "ISP_REG_PER_DIP_RANGE(0x%lx)!\n",
				pfn, length, ISP_REG_PER_DIP_RANGE);
			return -EAGAIN;
		}
		break;
	case SENINF_BASE_HW:
		if (length > 0x8000) {
			LOG_ERR(TMP_STR "SENINF_BASE_RANGE(0x%x)!\n",
				pfn, length, 0x4000);
			return -EAGAIN;
		}
		break;
	case MIPI_RX_BASE_HW:
		if (length > 0x12000) {
			LOG_ERR(TMP_STR "MIPI_RX_RANGE(0x%x)!\n",
				pfn, length, 0x12000);
			return -EAGAIN;
		}
		break;
	case GPIO_BASE_HW:
		if (length > 0x1000) {
			LOG_ERR(TMP_STR "GPIO_RX_RANGE(0x%x)!\n",
				pfn, length, 0x1000);
			return -EAGAIN;
		}
		break;
	default:
		LOG_ERR("Illegal starting HW addr for mmap!\n");
		return -EAGAIN;
	}
#undef TMP_STR
	if (remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff,
		pVma->vm_end - pVma->vm_start, pVma->vm_page_prot))
		return -EAGAIN;

	/*  */
	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/

static dev_t IspDevNo;
static struct cdev *pIspCharDrv;
static struct class *pIspClass;

static const struct file_operations IspFileOper = {
	.owner = THIS_MODULE,
	.open = ISP_open,
	.release = ISP_release,
	/* .flush       = mt_isp_flush, */
	.mmap = ISP_mmap,
	.unlocked_ioctl = ISP_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ISP_ioctl_compat,
#endif
};

/*******************************************************************************
 *
 ******************************************************************************/
static inline void ISP_UnregCharDev(void)
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
 ******************************************************************************/
static inline int ISP_RegCharDev(void)
{
	int Ret = 0;
	/*  */
	LOG_DBG("- E.\n");
	/*  */
	Ret = alloc_chrdev_region(&IspDevNo, 0, 1, ISP_DEV_NAME);
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
		ISP_UnregCharDev();


	/*      */

	LOG_DBG("- X.\n");
	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_probe(struct platform_device *pDev)
{
	int Ret = 0;
	/*    struct resource *pRes = NULL;*/
	int i = 0, j = 0;
	unsigned char n;
	unsigned int irq_info[3]; /* Record interrupts info from device tree */
	struct isp_device *_ispdev = NULL;

#ifdef CONFIG_OF
	struct isp_device *isp_dev;
	struct device *dev = NULL;
#endif

	LOG_INF("- E. ISP driver probe.\n");

	/* Get platform_device parameters */
#ifdef CONFIG_OF

	if (pDev == NULL) {
		dev_err(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}

	nr_isp_devs += 1;
	atomic_inc(&G_u4DevNodeCt);
#if 1
	_ispdev = krealloc(isp_devs, sizeof(struct isp_device) * nr_isp_devs,
		GFP_KERNEL);
	if (!_ispdev) {
		dev_err(&pDev->dev, "Unable to allocate isp_devs\n");
		return -ENOMEM;
	}
	isp_devs = _ispdev;


	#if defined(ISP_MET_READY)
	/*MET: met mmsys profile*/
	if (met_mmsys_config_isp_base_addr)
		met_mmsys_config_isp_base_addr((unsigned long *)isp_devs);
	#endif


#else
	/* WARNING: KREALLOC_ARG_REUSE: Reusing the krealloc arg is almost
	 * always a bug.
	 */
	isp_devs = KREALLOC(isp_devs, sizeof(struct isp_device) * nr_isp_devs,
		GFP_KERNEL);
	if (!isp_devs) {
		dev_err(&pDev->dev, "Unable to allocate isp_devs\n");
		return -ENOMEM;
	}
#endif

	isp_dev = &(isp_devs[nr_isp_devs - 1]);
	isp_dev->dev = &pDev->dev;

	/* iomap registers */
	isp_dev->regs = of_iomap(pDev->dev.of_node, 0);
	if (!isp_dev->regs) {
#define ERR_STR1 "Error: Unable to ioremap registers, of_iomap fail,"
#define ERR_STR2 " nr_isp_devs=%d, devnode(%s).\n"
		dev_info(&pDev->dev, ERR_STR1 ERR_STR2,
			nr_isp_devs, pDev->dev.of_node->name);
#undef ERR_STR1
#undef ERR_STR2
		return -ENOMEM;
	}

	LOG_INF("nr_isp_devs=%d, devnode(%s), map_addr=0x%lx\n",
		nr_isp_devs, pDev->dev.of_node->name,
		(unsigned long)isp_dev->regs);

	/* get IRQ ID and request IRQ */
	isp_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);

	if (isp_dev->irq > 0) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array(pDev->dev.of_node, "interrupts",
			irq_info, ARRAY_SIZE(irq_info))) {
			dev_err(&pDev->dev, "get irq flags from DTS fail!!\n");
			return -ENODEV;
		}

		for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
			if ((strcmp(pDev->dev.of_node->name,
				IRQ_CB_TBL[i].device_name) == 0) &&
				(IRQ_CB_TBL[i].isr_fp != NULL)) {
				Ret = request_irq(isp_dev->irq,
					(irq_handler_t)IRQ_CB_TBL[i].isr_fp,
					irq_info[2],
					(const char *)IRQ_CB_TBL[i].device_name,
					NULL);

				if (Ret) {
#define ERR_STR1 "request_irq fail, nr_isp_devs=%d, devnode(%s),"
#define ERR_STR2 " irq=%d, ISR: %s\n"
					dev_err(&pDev->dev,
						ERR_STR1 ERR_STR2,
						nr_isp_devs,
						pDev->dev.of_node->name,
						isp_dev->irq,
						IRQ_CB_TBL[i].device_name);
#undef ERR_STR1
#undef ERR_STR2
					return Ret;
				}
#define TMP_STR "nr_isp_devs=%d, devnode(%s), irq=%d, ISR: %s\n"
				LOG_INF(TMP_STR,
					nr_isp_devs, pDev->dev.of_node->name,
					isp_dev->irq,
					IRQ_CB_TBL[i].device_name);
#undef TMP_STR
				break;
			}
		}

		if (i >= ISP_IRQ_TYPE_AMOUNT) {
#define TMP_STR "No corresponding ISR!!: nr_isp_devs=%d, devnode(%s), irq=%d\n"
			LOG_INF(TMP_STR,
				nr_isp_devs, pDev->dev.of_node->name,
				isp_dev->irq);
#undef TMP_STR
		}


	} else {
		LOG_INF("No IRQ!!: nr_isp_devs=%d, devnode(%s), irq=%d\n",
			nr_isp_devs, pDev->dev.of_node->name, isp_dev->irq);
	}



	/* Only register char driver in the 1st time */
	if (nr_isp_devs == 1) {
		/* Register char driver */
		Ret = ISP_RegCharDev();
		if ((Ret)) {
			dev_err(&pDev->dev, "register char failed");
			return Ret;
		}



		/* Create class register */
		pIspClass = class_create(THIS_MODULE, "ispdrv");
		if (IS_ERR(pIspClass)) {
			Ret = PTR_ERR(pIspClass);
			LOG_ERR("Unable to create class, err = %d\n", Ret);
			goto EXIT;
		}
		dev = device_create(pIspClass, NULL, IspDevNo, NULL,
			ISP_DEV_NAME);

		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_info(&pDev->dev,
				"Failed to create device: /dev/%s, err = %d",
				ISP_DEV_NAME, Ret);
			goto EXIT;
		}

#endif

		/* Init spinlocks */
		spin_lock_init(&(IspInfo.SpinLockIspRef));
		spin_lock_init(&(IspInfo.SpinLockIsp));
		for (n = 0; n < ISP_IRQ_TYPE_AMOUNT; n++) {
			spin_lock_init(&(IspInfo.SpinLockIrq[n]));
			spin_lock_init(&(IspInfo.SpinLockIrqCnt[n]));
		}
		spin_lock_init(&(IspInfo.SpinLockRTBC));
		spin_lock_init(&(IspInfo.SpinLockClock));

		spin_lock_init(&(SpinLock_P2FrameList));
		spin_lock_init(&(SpinLockRegScen));
		spin_lock_init(&(SpinLock_UserKey));
		#ifdef ENABLE_KEEP_ION_HANDLE
		for (i = 0; i < ISP_DEV_NODE_NUM; i++) {
			if (gION_TBL[i].node != ISP_DEV_NODE_NUM) {
				for (n = 0; n < _dma_max_wr_; n++)
					spin_lock_init(&(gION_TBL[i].pLock[n]));
			}
		}
		#endif

#ifndef EP_NO_CLKMGR

#ifdef CONFIG_MTK_CLKMGR
#else
		/*CCF: Grab clock pointer (struct clk*) */

		isp_clk.ISP_SCP_SYS_DIS =
			devm_clk_get(&pDev->dev, "ISP_SCP_SYS_DIS");
		isp_clk.ISP_SCP_SYS_ISP =
			devm_clk_get(&pDev->dev, "ISP_SCP_SYS_ISP");
		isp_clk.ISP_SCP_SYS_CAM =
			devm_clk_get(&pDev->dev, "ISP_SCP_SYS_CAM");
		isp_clk.ISP_IMG_DIP = devm_clk_get(&pDev->dev, "IMG_DIP");
		isp_clk.ISP_CAM_CAMSYS =
			devm_clk_get(&pDev->dev, "CAMSYS_CAM_CGPDN");
		isp_clk.ISP_CAM_CAMTG =
			devm_clk_get(&pDev->dev, "CAMSYS_CAMTG_CGPDN");
		isp_clk.ISP_CAM_SENINF =
			devm_clk_get(&pDev->dev, "CAMSYS_SENINF_CGPDN");
		isp_clk.ISP_CAM_CAMSV0 =
			devm_clk_get(&pDev->dev, "CAMSYS_CAMSV0_CGPDN");
		isp_clk.ISP_CAM_CAMSV1 =
			devm_clk_get(&pDev->dev, "CAMSYS_CAMSV1_CGPDN");
		isp_clk.ISP_CAM_CAMSV2 =
			devm_clk_get(&pDev->dev, "CAMSYS_CAMSV2_CGPDN");

		if (IS_ERR(isp_clk.ISP_SCP_SYS_DIS)) {
			LOG_ERR("cannot get ISP_SCP_SYS_DIS clock\n");
			return PTR_ERR(isp_clk.ISP_SCP_SYS_DIS);
		}
		if (IS_ERR(isp_clk.ISP_SCP_SYS_ISP)) {
			LOG_ERR("cannot get ISP_SCP_SYS_ISP clock\n");
			return PTR_ERR(isp_clk.ISP_SCP_SYS_ISP);
		}
		if (IS_ERR(isp_clk.ISP_SCP_SYS_CAM)) {
			LOG_ERR("cannot get ISP_SCP_SYS_CAM clock\n");
			return PTR_ERR(isp_clk.ISP_SCP_SYS_CAM);
		}
		if (IS_ERR(isp_clk.ISP_IMG_DIP)) {
			LOG_ERR("cannot get IMG_DIP clock\n");
			return PTR_ERR(isp_clk.ISP_IMG_DIP);
		}
		if (IS_ERR(isp_clk.ISP_CAM_CAMSYS)) {
			LOG_ERR("cannot get CAMSYS_CAM_CGPDN clock\n");
			return PTR_ERR(isp_clk.ISP_CAM_CAMSYS);
		}
		if (IS_ERR(isp_clk.ISP_CAM_CAMTG)) {
			LOG_ERR("cannot get CAMSYS_CAMTG_CGPDN clock\n");
			return PTR_ERR(isp_clk.ISP_CAM_CAMTG);
		}
		if (IS_ERR(isp_clk.ISP_CAM_SENINF)) {
			LOG_ERR("cannot get CAMSYS_SENINF_CGPDN clock\n");
			return PTR_ERR(isp_clk.ISP_CAM_SENINF);
		}
		if (IS_ERR(isp_clk.ISP_CAM_CAMSV0)) {
			LOG_ERR("cannot get CAMSYS_CAMSV0_CGPDN clock\n");
			return PTR_ERR(isp_clk.ISP_CAM_CAMSV0);
		}
		if (IS_ERR(isp_clk.ISP_CAM_CAMSV1)) {
			LOG_ERR("cannot get CAMSYS_CAMSV1_CGPDN clock\n");
			return PTR_ERR(isp_clk.ISP_CAM_CAMSV1);
		}
		if (IS_ERR(isp_clk.ISP_CAM_CAMSV2)) {
			LOG_ERR("cannot get CAMSYS_CAMSV2_CGPDN clock\n");
			return PTR_ERR(isp_clk.ISP_CAM_CAMSV2);
		}

#endif
#endif
		/*  */
		for (i = 0 ; i < ISP_IRQ_TYPE_AMOUNT; i++)
			init_waitqueue_head(&IspInfo.WaitQueueHead[i]);

#ifdef CONFIG_PM_WAKELOCKS
		wakeup_source_init(&isp_wake_lock, "isp_lock_wakelock");
#else
		wake_lock_init(&isp_wake_lock, WAKE_LOCK_SUSPEND,
			"isp_lock_wakelock");
#endif

		/* enqueue/dequeue control in ihalpipe wrapper */
		init_waitqueue_head(&P2WaitQueueHead_WaitDeque);
		init_waitqueue_head(&P2WaitQueueHead_WaitFrame);
		init_waitqueue_head(&P2WaitQueueHead_WaitFrameEQDforDQ);

		for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(isp_tasklet[i].pIsp_tkt,
				isp_tasklet[i].tkt_cb, 0);

#if (ISP_BOTTOMHALF_WORKQ == 1)
		for (i = 0 ; i < ISP_IRQ_TYPE_AMOUNT; i++) {
			isp_workque[i].module = i;
			memset((void *)&(isp_workque[i].isp_bh_work), 0,
				sizeof(isp_workque[i].isp_bh_work));
			INIT_WORK(&(isp_workque[i].isp_bh_work),
				ISP_BH_Workqueue);
		}
#endif


		/* Init IspInfo*/
		spin_lock(&(IspInfo.SpinLockIspRef));
		IspInfo.UserCount = 0;
		spin_unlock(&(IspInfo.SpinLockIspRef));
		/*  */
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAM_A_ST][
			SIGNAL_INT] = INT_ST_MASK_CAM;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAM_B_ST][
			SIGNAL_INT] = INT_ST_MASK_CAM;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAMSV_0_ST][
			SIGNAL_INT] = INT_ST_MASK_CAMSV;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAMSV_1_ST][
			SIGNAL_INT] = INT_ST_MASK_CAMSV;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAMSV_2_ST][
			SIGNAL_INT] = INT_ST_MASK_CAMSV;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAMSV_3_ST][
			SIGNAL_INT] = INT_ST_MASK_CAMSV;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAMSV_4_ST][
			SIGNAL_INT] = INT_ST_MASK_CAMSV;
		IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT_CAMSV_5_ST][
			SIGNAL_INT] = INT_ST_MASK_CAMSV;
		/* only cam have warning mask */
		IspInfo.IrqInfo.WarnMask[ISP_IRQ_TYPE_INT_CAM_A_ST][
			SIGNAL_INT] = INT_ST_MASK_CAM_WARN;
		IspInfo.IrqInfo.WarnMask[ISP_IRQ_TYPE_INT_CAM_B_ST][
			SIGNAL_INT] = INT_ST_MASK_CAM_WARN;
		/*  */
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAM_A_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAM_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAM_B_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAM_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAMSV_0_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAMSV_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAMSV_1_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAMSV_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAMSV_2_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAMSV_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAMSV_3_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAMSV_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAMSV_4_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAMSV_ERR;
		IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT_CAMSV_5_ST][
			SIGNAL_INT]  = INT_ST_MASK_CAMSV_ERR;

		/* Init IrqCntInfo */
		for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
			for (j = 0; j < ISP_ISR_MAX_NUM; j++) {
				IspInfo.IrqCntInfo.m_err_int_cnt[i][j] = 0;
				IspInfo.IrqCntInfo.m_warn_int_cnt[i][j] = 0;
			}
			IspInfo.IrqCntInfo.m_err_int_mark[i] = 0;
			IspInfo.IrqCntInfo.m_warn_int_mark[i] = 0;

			IspInfo.IrqCntInfo.m_int_usec[i] = 0;
		}


EXIT:
		if (Ret < 0)
			ISP_UnregCharDev();

	}

	LOG_INF("- X. ISP driver probe.\n");

	return Ret;
}

/*******************************************************************************
 * Called when the device is being detached from the driver
 ******************************************************************************/
static int ISP_remove(struct platform_device *pDev)
{
	/*    struct resource *pRes;*/
	int IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	/* unregister char driver. */
	ISP_UnregCharDev();

	/* Release IRQ */
	disable_irq(IspInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* kill tasklet */
	for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(isp_tasklet[i].pIsp_tkt);

#if 0
	/* free all registered irq(child nodes) */
	ISP_UnRegister_AllregIrq();
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
			accessNode = ((REG_IRQ_NODE *)((char *)__mptr -
				offsetof(REG_IRQ_NODE, list)));
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

static int ISP_suspend(
	struct platform_device *pDev,
	pm_message_t            Mesg
)
{
	unsigned int regVal;
	int IrqType, ret, module;
	char moduleName[128];

	unsigned int regTGSt, loopCnt;
	struct ISP_WAIT_IRQ_STRUCT waitirq;
	ktime_t             time;
	unsigned long long  sec = 0, m_sec = 0;
	unsigned long long  timeoutMs = 500000000;/*500ms*/

	ret = 0;
	module = -1;
	strncpy(moduleName, pDev->dev.of_node->name, 127);

	/* update device node count*/
	atomic_dec(&G_u4DevNodeCt);

	/* Check clock counter instead of check IspInfo.UserCount
	 *  for ensuring current clocks are on or off.
	 */
	spin_lock(&(IspInfo.SpinLockClock));
	if (!G_u4EnableClockCount) {
		spin_unlock(&(IspInfo.SpinLockClock));
		/* Only print cama log */
#define TMP_STR "%s suspend - X. UserCount=%d, wakelock:%d, clk:%d, devct:%d\n"
		if (strcmp(moduleName, IRQ_CB_TBL[
			ISP_IRQ_TYPE_INT_CAM_A_ST].device_name) == 0) {
			LOG_DBG(TMP_STR, moduleName, IspInfo.UserCount,
				g_WaitLockCt, G_u4EnableClockCount,
				atomic_read(&G_u4DevNodeCt));
		} else if (IspInfo.UserCount != 0) {

			LOG_INF(TMP_STR, moduleName, IspInfo.UserCount,
				g_WaitLockCt, G_u4EnableClockCount,
				atomic_read(&G_u4DevNodeCt));
		}
#undef TMP_STR
		return ret;
	}
	spin_unlock(&(IspInfo.SpinLockClock));

	for (IrqType = 0; IrqType < ISP_IRQ_TYPE_AMOUNT; IrqType++) {
		if (strcmp(moduleName, IRQ_CB_TBL[IrqType].device_name) == 0)
			break;
	}

	switch (IrqType) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
		module = ISP_CAM_A_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		module = ISP_CAM_B_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
		module = ISP_CAMSV0_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
		module = ISP_CAMSV1_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
		module = ISP_CAMSV2_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
		module = ISP_CAMSV3_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
		module = ISP_CAMSV4_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
		module = ISP_CAMSV5_IDX;
		break;
	case ISP_IRQ_TYPE_AMOUNT:
		LOG_ERR("dev name is not found (%s)", moduleName);
		break;
	case ISP_IRQ_TYPE_INT_UNI_A_ST:
	default:
		/*don nothing*/
		break;
	}

	if (module < 0)
		goto EXIT;

	regVal = ISP_RD32(CAM_REG_TG_VF_CON(module));
	/*LOG_DBG("%s: Rs_TG(0x%08x)\n", moduleName, regVal);*/

	if (regVal & 0x01) {
		LOG_INF("%s_suspend,disable VF,wakelock:%d,clk:%d,devct:%d\n",
			moduleName, g_WaitLockCt, G_u4EnableClockCount,
			atomic_read(&G_u4DevNodeCt));
		SuspnedRecord[module] = 1;
		/* disable VF */
		ISP_WR32(CAM_REG_TG_VF_CON(module), (regVal & (~0x01)));

		/* wait TG idle*/
		loopCnt = 3;
		waitirq.Type = IrqType;
		waitirq.EventInfo.Clear = ISP_IRQ_CLEAR_WAIT;
		waitirq.EventInfo.Status = VS_INT_ST;
		waitirq.EventInfo.St_type = SIGNAL_INT;
		waitirq.EventInfo.Timeout = 0x100;
		waitirq.EventInfo.UserKey = 0x0;
		waitirq.bDumpReg = 0;

		do {
			regTGSt = (ISP_RD32(CAM_REG_TG_INTER_ST(module)) &
				0x00003F00) >> 8;
			if (regTGSt == 1)
				break;

			LOG_INF("%s: wait 1VD (%d)\n", moduleName, loopCnt);
			ret = ISP_WaitIrq(&waitirq);
			/* 1st wait is clear wait, others are non-clear wait */
			waitirq.EventInfo.Clear = ISP_IRQ_CLEAR_NONE;
		} while (--loopCnt);

		if (-ERESTARTSYS == ret) {
			LOG_INF("%s: interrupt by system signal, wait idle\n",
				moduleName);
			/* timer*/
			time = ktime_get();
			m_sec = time.tv64;

			while (regTGSt != 1) {
				regTGSt =
					(ISP_RD32(CAM_REG_TG_INTER_ST(module)) &
					0x00003F00) >> 8;
				/*timer*/
				time = ktime_get();
				sec = time.tv64;
				/* wait time>timeoutMs, break */
				if ((sec - m_sec) > timeoutMs)
					break;
			}
			if (regTGSt == 1)
				LOG_INF("%s: wait idle done\n", moduleName);
			else
				LOG_INF("%s: wait idle timeout(%lld)\n",
					moduleName, (sec - m_sec));
		}

		/*backup: frame CNT
		 * After VF enabled, The frame count will be 0 at next VD;
		 * if it has P1_DON after setting vf disabled, g_BkReg no
		 * need to add 1.
		 */
		regTGSt = ISP_RD32_TG_CAM_FRM_CNT(IrqType, module);
		g_BkReg[IrqType].CAM_TG_INTER_ST = regTGSt;
		regVal = ISP_RD32(CAM_REG_TG_SEN_MODE(module));
		ISP_WR32(CAM_REG_TG_SEN_MODE(module), (regVal & (~0x01)));
	} else {
		LOG_INF("%s_suspend,wakelock:%d,clk:%d,devct:%d\n", moduleName,
			g_WaitLockCt,
			G_u4EnableClockCount, atomic_read(&G_u4DevNodeCt));
		SuspnedRecord[module] = 0;
	}

EXIT:
	/* last dev node will disable clk "G_u4EnableClockCount" times */
	if (!atomic_read(&G_u4DevNodeCt)) {
		spin_lock(&(IspInfo.SpinLockClock));
		loopCnt = G_u4EnableClockCount;
		spin_unlock(&(IspInfo.SpinLockClock));

		LOG_INF("%s - X. wakelock:%d, last dev node,disable clk:%d\n",
			moduleName, g_WaitLockCt, loopCnt);
		while (loopCnt > 0) {
			ISP_EnableClock(MFALSE);
			loopCnt--;
		}
	}

	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
static int ISP_resume(struct platform_device *pDev)
{
	unsigned int regVal;
	int IrqType, ret, module;
	char moduleName[128];

	ret = 0;
	module = -1;
	strncpy(moduleName, pDev->dev.of_node->name, 127);

	/* update device node count*/
	atomic_inc(&G_u4DevNodeCt);

	if (IspInfo.UserCount == 0) {
		/* Only print cama log */
		if (strcmp(moduleName,
			IRQ_CB_TBL[ISP_IRQ_TYPE_INT_CAM_A_ST].device_name) == 0)
			LOG_DBG("%s - X. UserCount=0\n", moduleName);

		return 0;
	}

	for (IrqType = 0; IrqType < ISP_IRQ_TYPE_AMOUNT; IrqType++) {
		if (strcmp(moduleName, IRQ_CB_TBL[IrqType].device_name) == 0)
			break;
	}

	switch (IrqType) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
		module = ISP_CAM_A_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		module = ISP_CAM_B_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
		module = ISP_CAMSV0_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
		module = ISP_CAMSV1_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
		module = ISP_CAMSV2_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
		module = ISP_CAMSV3_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
		module = ISP_CAMSV4_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
		module = ISP_CAMSV5_IDX;
		break;
	case ISP_IRQ_TYPE_AMOUNT:
		LOG_ERR("dev name is not found (%s)", moduleName);
		break;
	case ISP_IRQ_TYPE_INT_UNI_A_ST:
	default:
		/*don nothing*/
		break;
	}

	if (module < 0)
		return ret;

	ISP_EnableClock(MTRUE);

	if (SuspnedRecord[module]) {
		LOG_INF("%s_resume,enable VF,wakelock:%d,clk:%d,devct:%d\n",
			moduleName,
			g_WaitLockCt, G_u4EnableClockCount,
			atomic_read(&G_u4DevNodeCt));
		SuspnedRecord[module] = 0;

		/*cmos*/
		regVal = ISP_RD32(CAM_REG_TG_SEN_MODE(module));
		ISP_WR32(CAM_REG_TG_SEN_MODE(module), (regVal | 0x01));
		/*vf*/
		regVal = ISP_RD32(CAM_REG_TG_VF_CON(module));
		ISP_WR32(CAM_REG_TG_VF_CON(module), (regVal | 0x01));
	} else {
		LOG_INF("%s_resume,wakelock:%d,clk:%d,devct:%d\n", moduleName,
			g_WaitLockCt, G_u4EnableClockCount,
			atomic_read(&G_u4DevNodeCt));
	}

	return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int ISP_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	/*pr_debug("calling %s()\n", __func__);*/

	return ISP_suspend(pdev, PMSG_SUSPEND);
}

int ISP_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	/*pr_debug("calling %s()\n", __func__);*/

	return ISP_resume(pdev);
}

/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
int ISP_pm_restore_noirq(struct device *device)
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
#define ISP_pm_suspend NULL
#define ISP_pm_resume  NULL
#define ISP_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/

const struct dev_pm_ops ISP_pm_ops = {
	.suspend = ISP_pm_suspend,
	.resume = ISP_pm_resume,
	.freeze = ISP_pm_suspend,
	.thaw = ISP_pm_resume,
	.poweroff = ISP_pm_suspend,
	.restore = ISP_pm_resume,
	.restore_noirq = ISP_pm_restore_noirq,
};


/*******************************************************************************
 *
 ******************************************************************************/
static struct platform_driver IspDriver = {
	.probe   = ISP_probe,
	.remove  = ISP_remove,
	.suspend = ISP_suspend,
	.resume  = ISP_resume,
	.driver  = {
		.name  = ISP_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = isp_of_ids,
#endif
#ifdef CONFIG_PM
		.pm     = &ISP_pm_ops,
#endif
	}
};

/*******************************************************************************
 *
 ******************************************************************************/
static ssize_t ISP_DumpRegToProc(
	struct file *pFile,
	char *pStart,
	size_t off,
	loff_t *Count)
{
	LOG_ERR("%s: Not implement\n", __func__);
	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
static ssize_t ISP_RegDebug(
	struct file *pFile,
	const char *pBuffer,
	size_t Count,
	loff_t *pData)
{
	LOG_ERR("%s: Not implement\n", __func__);
	return 0;
}

static ssize_t CAMIO_DumpRegToProc(
	struct file *pFile,
	char *pStart,
	size_t off,
	loff_t *Count)
{
	LOG_ERR("%s: Not implement\n", __func__);
	return 0;
}


/*******************************************************************************
 *
 ******************************************************************************/
static ssize_t CAMIO_RegDebug(
	struct file *pFile,
	const char *pBuffer,
	size_t Count,
	loff_t *pData)
{
	LOG_ERR("%s: Not implement\n", __func__);
	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
static int isp_p2_ke_dump_read(struct seq_file *m, void *v)
{
#ifdef AEE_DUMP_REDUCE_MEMORY
	int i;

	LOG_INF("isp p2 ke dump start!! g_bDumpPhyISPBuf:%d\n",
		g_bDumpPhyISPBuf);
	LOG_INF("g_bDumpPhyISPBuf:%d, g_tdriaddr:0x%x, g_cmdqaddr:0x%x\n",
		g_bDumpPhyISPBuf, g_tdriaddr, g_cmdqaddr);
	seq_puts(m, "============ isp p2 ke dump register============\n");

#define TMP_STR1 "===isp p2 you can trust below info: g_bDumpPhyISPBuf:%d===\n"
#define TMP_STR2 "===isp p2 g_bDumpPhyISPBuf:%d, g_tdriaddr:0x%x,"
#define TMP_STR3 " g_cmdqaddr:0x%x===\n"
	seq_printf(m, TMP_STR1, g_bDumpPhyISPBuf);
	seq_printf(m, TMP_STR2 TMP_STR3, g_bDumpPhyISPBuf, g_tdriaddr,
		g_cmdqaddr);
#undef TMP_STR1
#undef TMP_STR2
#undef TMP_STR3
	seq_puts(m, "===isp p2 hw physical register===\n");
	if (g_bDumpPhyISPBuf == MFALSE)
		return 0;
	mutex_lock(&gDipMutex);  /* Protect the Multi Process */
#define TMP_STR "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n"
	if (g_pPhyISPBuffer != NULL) {
		for (i = 0; i < (ISP_DIP_PHYSICAL_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, TMP_STR,
				DIP_A_BASE_HW + 4 * i,
				(unsigned int)g_pPhyISPBuffer[i],
				DIP_A_BASE_HW + 4 * (i + 1),
				(unsigned int)g_pPhyISPBuffer[i + 1],
				DIP_A_BASE_HW + 4 * (i + 2),
				(unsigned int)g_pPhyISPBuffer[i + 2],
				DIP_A_BASE_HW + 4 * (i + 3),
				(unsigned int)g_pPhyISPBuffer[i + 3]);
		}
	} else {
		LOG_INF("g_pPhyISPBuffer:(0x%pK)\n", g_pPhyISPBuffer);
	}
	seq_puts(m, "===isp p2 tpipe buffer Info===\n");
	if (g_pKWTpipeBuffer != NULL) {
		for (i = 0; i < (MAX_ISP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
			seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
					(unsigned int)g_pKWTpipeBuffer[i],
					(unsigned int)g_pKWTpipeBuffer[i+1],
					(unsigned int)g_pKWTpipeBuffer[i+2],
					(unsigned int)g_pKWTpipeBuffer[i+3]);
		}
	}
	seq_puts(m, "===isp p2 cmdq buffer Info===\n");
	if (g_pKWCmdqBuffer != NULL) {
		for (i = 0; i < (MAX_ISP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
			seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
					(unsigned int)g_pKWCmdqBuffer[i],
					(unsigned int)g_pKWCmdqBuffer[i+1],
					(unsigned int)g_pKWCmdqBuffer[i+2],
					(unsigned int)g_pKWCmdqBuffer[i+3]);
		}
	}
	seq_puts(m, "===isp p2 vir isp buffer Info===\n");
	if (g_pKWVirISPBuffer != NULL) {
		for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, TMP_STR,
				DIP_A_BASE_HW + 4 * i,
				(unsigned int)g_pKWVirISPBuffer[i],
				DIP_A_BASE_HW + 4 * (i + 1),
				(unsigned int)g_pKWVirISPBuffer[i + 1],
				DIP_A_BASE_HW + 4 * (i + 2),
				(unsigned int)g_pKWVirISPBuffer[i + 2],
				DIP_A_BASE_HW + 4 * (i + 3),
				(unsigned int)g_pKWVirISPBuffer[i + 3]);
		}
	}
	seq_puts(m, "============ isp p2 ke dump debug ============\n");
	mutex_unlock(&gDipMutex);
	LOG_INF("isp p2 ke dump end\n");
#else
	int i;

	mutex_lock(&gDipMutex);  /* Protect the Multi Process */
	seq_puts(m, "============ isp p2 ke dump register============\n");
	seq_puts(m, "isp p2 hw physical register\n");
	for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, TMP_STR,
			DIP_A_BASE_HW + 4 * i,
			(unsigned int)g_PhyISPBuffer[i],
			DIP_A_BASE_HW + 4 * (i + 1),
			(unsigned int)g_PhyISPBuffer[i + 1],
			DIP_A_BASE_HW + 4 * (i + 2),
			(unsigned int)g_PhyISPBuffer[i + 2],
			DIP_A_BASE_HW + 4 * (i + 3),
			(unsigned int)g_PhyISPBuffer[i + 3]);
	}
	seq_puts(m, "isp p2 tpipe buffer Info\n");
	for (i = 0; i < (MAX_ISP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
		seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
				(unsigned int)g_KWTpipeBuffer[i],
				(unsigned int)g_KWTpipeBuffer[i+1],
				(unsigned int)g_KWTpipeBuffer[i+2],
				(unsigned int)g_KWTpipeBuffer[i+3]);
	}
	seq_puts(m, "isp p2 cmdq buffer Info\n");
	for (i = 0; i < (MAX_ISP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
				(unsigned int)g_KWCmdqBuffer[i],
				(unsigned int)g_KWCmdqBuffer[i+1],
				(unsigned int)g_KWCmdqBuffer[i+2],
				(unsigned int)g_KWCmdqBuffer[i+3]);
	}
	seq_puts(m, "isp p2 vir isp buffer Info\n");
	for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, TMP_STR,
			DIP_A_BASE_HW + 4 * i,
			(unsigned int)g_KWVirISPBuffer[i],
			DIP_A_BASE_HW + 4 * (i + 1),
			(unsigned int)g_KWVirISPBuffer[i + 1],
			DIP_A_BASE_HW + 4 * (i + 2),
			(unsigned int)g_KWVirISPBuffer[i + 2],
			DIP_A_BASE_HW + 4 * (i + 3),
			(unsigned int)g_KWVirISPBuffer[i + 3]);
	}
#undef TMP_STR
	mutex_unlock(&gDipMutex);
	seq_puts(m, "============ isp p2 ke dump debug ============\n");
#endif
	return 0;
}
static int proc_isp_p2_ke_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_p2_ke_dump_read, NULL);
}
static const struct file_operations isp_p2_ke_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_isp_p2_ke_dump_open,
	.read = seq_read,
};

/*******************************************************************************
 *
 ******************************************************************************/
static int isp_p2_dump_read(struct seq_file *m, void *v)
{
#ifdef AEE_DUMP_REDUCE_MEMORY
	int i;

#define TMP_STR1 "isp p2 ne dump start!! g_bUserBufIsReady:%d,"
#define TMP_STR2 "g_bIonBufferAllocated:%d\n"
#define TMP_STR3 "isp p2 g_bDumpPhyB:%d, tdriadd:0x%x,"
#define TMP_STR4 " imgiadd:0x%x, dmgiadd:0x%x\n"

	LOG_INF(TMP_STR1 TMP_STR2, g_bUserBufIsReady, g_bIonBufferAllocated);
	LOG_INF(TMP_STR3 TMP_STR4, g_bDumpPhyISPBuf, g_dumpInfo.tdri_baseaddr,
		g_dumpInfo.imgi_baseaddr, g_dumpInfo.dmgi_baseaddr);
#undef TMP_STR1
#undef TMP_STR2
#undef TMP_STR3
#undef TMP_STR4
	seq_puts(m, "============ isp p2 ne dump register ============\n");
	seq_printf(m, "==isp p2 you can trust below info:UserBufIsReady:%d==\n",
		g_bUserBufIsReady);
#define TMP_STR1 "==isp p2 g_bDumpPhyB:%d, tdriadd:0x%x,"
#define TMP_STR2 " imgiadd:0x%x, dmgiadd:0x%x==\n"
	seq_printf(m, TMP_STR1 TMP_STR2, g_bDumpPhyISPBuf,
		g_dumpInfo.tdri_baseaddr, g_dumpInfo.imgi_baseaddr,
		g_dumpInfo.dmgi_baseaddr);
#undef TMP_STR1
#undef TMP_STR2
	seq_puts(m, "==isp p2 hw physical register==\n");
	if (g_bUserBufIsReady == MFALSE)
		return 0;
	mutex_lock(&gDipMutex);  /* Protect the Multi Process */

#define TMP_STR "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n"
	if (g_pPhyISPBuffer != NULL) {
		for (i = 0; i < (ISP_DIP_PHYSICAL_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, TMP_STR,
				DIP_A_BASE_HW + 4 * i,
				(unsigned int)g_pPhyISPBuffer[i],
				DIP_A_BASE_HW + 4 * (i + 1),
				(unsigned int)g_pPhyISPBuffer[i + 1],
				DIP_A_BASE_HW + 4 * (i + 2),
				(unsigned int)g_pPhyISPBuffer[i + 2],
				DIP_A_BASE_HW + 4 * (i + 3),
				(unsigned int)g_pPhyISPBuffer[i + 3]);
		}
	} else {
		LOG_INF("g_pPhyISPBuffer:(0x%pK)\n", g_pPhyISPBuffer);
	}
	seq_puts(m, "===isp p2 tpipe buffer Info===\n");
	if (g_pTpipeBuffer != NULL) {
		for (i = 0; i < (MAX_ISP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
			seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
					(unsigned int)g_pTpipeBuffer[i],
					(unsigned int)g_pTpipeBuffer[i+1],
					(unsigned int)g_pTpipeBuffer[i+2],
					(unsigned int)g_pTpipeBuffer[i+3]);
		}
	}
	seq_puts(m, "===isp p2 cmdq buffer Info===\n");
	if (g_pCmdqBuffer != NULL) {
		for (i = 0; i < (MAX_ISP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
			seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
					(unsigned int)g_pCmdqBuffer[i],
					(unsigned int)g_pCmdqBuffer[i+1],
					(unsigned int)g_pCmdqBuffer[i+2],
					(unsigned int)g_pCmdqBuffer[i+3]);
		}
	}
	seq_puts(m, "===isp p2 vir isp buffer Info===\n");
	if (g_pVirISPBuffer != NULL) {
		for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, TMP_STR,
				DIP_A_BASE_HW + 4 * i,
				(unsigned int)g_pVirISPBuffer[i],
				DIP_A_BASE_HW + 4 * (i + 1),
				(unsigned int)g_pVirISPBuffer[i + 1],
				DIP_A_BASE_HW + 4 * (i + 2),
				(unsigned int)g_pVirISPBuffer[i + 2],
				DIP_A_BASE_HW + 4 * (i + 3),
				(unsigned int)g_pVirISPBuffer[i + 3]);
		}
	}
	seq_puts(m, "===isp p2 tuning buffer Info===\n");
	if (g_pTuningBuffer != NULL) {
		for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
			seq_printf(m, TMP_STR,
				DIP_A_BASE_HW + 4 * i,
				(unsigned int)g_pTuningBuffer[i],
				DIP_A_BASE_HW + 4 * (i + 1),
				(unsigned int)g_pTuningBuffer[i + 1],
				DIP_A_BASE_HW + 4 * (i + 2),
				(unsigned int)g_pTuningBuffer[i + 2],
				DIP_A_BASE_HW + 4 * (i + 3),
				(unsigned int)g_pTuningBuffer[i + 3]);
		}
	}
#undef TMP_STR
	seq_puts(m, "============ isp p2 ne dump debug ============\n");
	LOG_INF("isp p2 ne dump end\n");
#else
	int i;

	seq_puts(m, "============ isp p2 ne dump register============\n");
	seq_puts(m, "isp p2 hw physical register\n");

#define TMP_STR "(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)(0x%08X,0x%08X)\n"

	for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, TMP_STR,
			DIP_A_BASE_HW + 4 * i,
			(unsigned int)g_PhyISPBuffer[i],
			DIP_A_BASE_HW + 4 * (i + 1),
			(unsigned int)g_PhyISPBuffer[i + 1],
			DIP_A_BASE_HW + 4 * (i + 2),
			(unsigned int)g_PhyISPBuffer[i + 2],
			DIP_A_BASE_HW + 4 * (i + 3),
			(unsigned int)g_PhyISPBuffer[i + 3]);
	}

	seq_puts(m, "isp p2 tpipe buffer Info\n");
	for (i = 0; i < (MAX_ISP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
		seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
			   (unsigned int)g_TpipeBuffer[i],
			   (unsigned int)g_TpipeBuffer[i + 1],
			   (unsigned int)g_TpipeBuffer[i + 2],
			   (unsigned int)g_TpipeBuffer[i + 3]);
	}

	seq_puts(m, "isp p2 cmdq buffer Info\n");
	for (i = 0; i < (MAX_ISP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
			   (unsigned int)g_CmdqBuffer[i],
			   (unsigned int)g_CmdqBuffer[i + 1],
			   (unsigned int)g_CmdqBuffer[i + 2],
			   (unsigned int)g_CmdqBuffer[i + 3]);
	}

	seq_puts(m, "isp p2 vir isp buffer Info\n");
	for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, TMP_STR,
			DIP_A_BASE_HW + 4 * i,
			(unsigned int)g_VirISPBuffer[i],
			DIP_A_BASE_HW + 4 * (i + 1),
			(unsigned int)g_VirISPBuffer[i + 1],
			DIP_A_BASE_HW + 4 * (i + 2),
			(unsigned int)g_VirISPBuffer[i + 2],
			DIP_A_BASE_HW + 4 * (i + 3),
			(unsigned int)g_VirISPBuffer[i + 3]);
	}
	seq_puts(m, "isp p2 tuning buffer Info\n");
	for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, TMP_STR,
			DIP_A_BASE_HW + 4 * i,
			(unsigned int)g_TuningBuffer[i],
			DIP_A_BASE_HW + 4 * (i + 1),
			(unsigned int)g_TuningBuffer[i + 1],
			DIP_A_BASE_HW + 4 * (i + 2),
			(unsigned int)g_TuningBuffer[i + 2],
			DIP_A_BASE_HW + 4 * (i + 3),
			(unsigned int)g_TuningBuffer[i + 3]);
	}
	seq_puts(m, "============ kernel warning ============\n");
	seq_puts(m, "ke:isp p2 tpipe buffer Info\n");
	for (i = 0; i < (MAX_ISP_TILE_TDR_HEX_NO >> 2); i = i + 4) {
		seq_printf(m, "0x%08X\n0x%08X\n0x%08X\n0x%08X\n",
				(unsigned int)g_KWTpipeBuffer[i],
				(unsigned int)g_KWTpipeBuffer[i+1],
				(unsigned int)g_KWTpipeBuffer[i+2],
				(unsigned int)g_KWTpipeBuffer[i+3]);
	}
	seq_puts(m, "ke:isp p2 cmdq buffer Info\n");
	for (i = 0; i < (MAX_ISP_CMDQ_BUFFER_SIZE >> 2); i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X 0x%08X 0x%08X]\n",
				(unsigned int)g_KWCmdqBuffer[i],
				(unsigned int)g_KWCmdqBuffer[i+1],
				(unsigned int)g_KWCmdqBuffer[i+2],
				(unsigned int)g_KWCmdqBuffer[i+3]);
	}
	seq_puts(m, "ke:isp p2 vir isp buffer Info\n");
	for (i = 0; i < (ISP_DIP_REG_SIZE >> 2); i = i + 4) {
		seq_printf(m, TMP_STR,
			DIP_A_BASE_HW + 4 * i,
			(unsigned int)g_KWVirISPBuffer[i],
			DIP_A_BASE_HW + 4 * (i + 1),
			(unsigned int)g_KWVirISPBuffer[i + 1],
			DIP_A_BASE_HW + 4 * (i + 2),
			(unsigned int)g_KWVirISPBuffer[i + 2],
			DIP_A_BASE_HW + 4 * (i + 3),
			(unsigned int)g_KWVirISPBuffer[i + 3]);
	}
#undef TMP_STR
	mutex_unlock(&gDipMutex);
	seq_puts(m, "============ isp p2 ne dump debug ============\n");
#endif
	return 0;
}

static int proc_isp_p2_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_p2_dump_read, NULL);
}

static const struct file_operations isp_p2_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_isp_p2_dump_open,
	.read = seq_read,
};
/*******************************************************************************
 *
 ******************************************************************************/
static const struct file_operations fcameraisp_proc_fops = {
	.read = ISP_DumpRegToProc,
	.write = ISP_RegDebug,
};
static const struct file_operations fcameraio_proc_fops = {
	.read = CAMIO_DumpRegToProc,
	.write = CAMIO_RegDebug,
};
/*******************************************************************************
 *
 ******************************************************************************/

static int __init ISP_Init(void)
{
	int Ret = 0, j;
	void *tmp;
	struct device_node *node = NULL;
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *isp_p2_dir;

	int i;
	/*  */
	LOG_DBG("- E.");
	/*  */
	atomic_set(&G_u4DevNodeCt, 0);
	/*  */
	Ret = platform_driver_register(&IspDriver);
	if ((Ret) < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}
	/*  */

	/* Use of_find_compatible_node() sensor registers from device tree */
	/* Don't use compatitble define in probe(). Otherwise, probe() of
	 * Seninf driver cannot be called.
	 */
	#if (SMI_LARB_MMU_CTL == 1)
	do {
		char *comp_str = NULL;

		comp_str = kmalloc(64, GFP_KERNEL);
		if (comp_str == NULL) {
			LOG_ERR("kmalloc failed for finding compatible\n");
			break;
		}

		for (i = 0; i < ARRAY_SIZE(SMI_LARB_BASE); i++) {

			snprintf(comp_str, 64, "mediatek,smi_larb%d", i);
			LOG_INF("Finding SMI_LARB compatible: %s\n", comp_str);

			node = of_find_compatible_node(NULL, NULL, comp_str);
			if (!node) {
				LOG_ERR("find %s node failed!!!\n", comp_str);
				SMI_LARB_BASE[i] = 0;
				continue;
			}
			SMI_LARB_BASE[i] = of_iomap(node, 0);
			if (!SMI_LARB_BASE[i]) {
#define ERR_STR "unable to map ISP_SENINF0_BASE registers!!!\n"
				LOG_ERR(ERR_STR);
#undef ERR_STR
				break;
			}
			LOG_INF("SMI_LARB%d_BASE: %p\n", i, SMI_LARB_BASE[i]);
		}

		/* if (comp_str) coverity: no need if, kfree is safe */
		kfree(comp_str);
	} while (0);
	#endif
	node = of_find_compatible_node(NULL, NULL, "mediatek,seninf1");
	if (!node) {
		LOG_ERR("find mediatek,seninf1 node failed!!!\n");
		return -ENODEV;
	}
	ISP_SENINF0_BASE = of_iomap(node, 0);
	if (!ISP_SENINF0_BASE) {
		LOG_ERR("unable to map ISP_SENINF0_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_SENINF0_BASE: %p\n", ISP_SENINF0_BASE);

	node = of_find_compatible_node(NULL, NULL, "mediatek,seninf2");
	if (!node) {
		LOG_ERR("find mediatek,seninf2 node failed!!!\n");
		return -ENODEV;
	}
	ISP_SENINF1_BASE = of_iomap(node, 0);
	if (!ISP_SENINF1_BASE) {
		LOG_ERR("unable to map ISP_SENINF1_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_SENINF1_BASE: %p\n", ISP_SENINF1_BASE);

	node = of_find_compatible_node(NULL, NULL, "mediatek,seninf3");
	if (!node) {
		LOG_ERR("find mediatek,seninf3 node failed!!!\n");
		return -ENODEV;
	}
	ISP_SENINF2_BASE = of_iomap(node, 0);
	if (!ISP_SENINF2_BASE) {
		LOG_ERR("unable to map ISP_SENINF2_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_SENINF2_BASE: %p\n", ISP_SENINF2_BASE);

	node = of_find_compatible_node(NULL, NULL, "mediatek,seninf4");
	if (!node) {
		LOG_ERR("find mediatek,seninf4 node failed!!!\n");
		return -ENODEV;
	}
	ISP_SENINF3_BASE = of_iomap(node, 0);
	if (!ISP_SENINF3_BASE) {
		LOG_ERR("unable to map ISP_SENINF3_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_SENINF3_BASE: %p\n", ISP_SENINF3_BASE);

	node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
	if (!node) {
		LOG_ERR("find mediatek,apmixed node failed!!!\n");
		return -ENODEV;
	}
	CLOCK_CELL_BASE = of_iomap(node, 0);
	if (!CLOCK_CELL_BASE) {
		LOG_ERR("unable to map CLOCK_CELL_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("CLOCK_CELL_BASE: %p\n", CLOCK_CELL_BASE);

	node = of_find_compatible_node(NULL, NULL, "mediatek,mmsys_config");
	if (!node) {
		LOG_ERR("find mmsys_config node failed!!!\n");
		return -ENODEV;
	}
	ISP_MMSYS_CONFIG_BASE = of_iomap(node, 0);
	if (!ISP_MMSYS_CONFIG_BASE) {
		LOG_ERR("unable to map ISP_MMSYS_CONFIG_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_MMSYS_CONFIG_BASE: %p\n", ISP_MMSYS_CONFIG_BASE);

	/* FIX-ME: linux-3.10 procfs API changed */
	proc_create("driver/isp_reg", 0000, NULL, &fcameraisp_proc_fops);
	proc_create("driver/camio_reg", 0000, NULL, &fcameraio_proc_fops);

	isp_p2_dir = proc_mkdir("isp_p2", NULL);
	if (!isp_p2_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/isp_p2\n", __func__);
		return 0;
	}
	proc_entry = proc_create("isp_p2_dump", 0444, isp_p2_dir,
		&isp_p2_dump_proc_fops);
	proc_entry = proc_create("isp_p2_kedump", 0444, isp_p2_dir,
		&isp_p2_ke_dump_proc_fops);
	for (j = 0; j < ISP_IRQ_TYPE_AMOUNT; j++) {
		switch (j) {
		case ISP_IRQ_TYPE_INT_CAM_A_ST:
		case ISP_IRQ_TYPE_INT_CAM_B_ST:
		case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
		case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
		case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
		case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
		case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
		case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
			if (sizeof(struct ISP_RT_BUF_STRUCT) >
				((RT_BUF_TBL_NPAGES) * PAGE_SIZE)) {
				i = 0;
				while (i < sizeof(struct ISP_RT_BUF_STRUCT))
					i += PAGE_SIZE;

				pBuf_kmalloc[j] = kmalloc(i + 2 * PAGE_SIZE,
					GFP_KERNEL);
				if ((pBuf_kmalloc[j]) == NULL) {
					LOG_ERR("mem not enough\n");
					return -ENOMEM;
				}
				memset(pBuf_kmalloc[j], 0x00, i);
				Tbl_RTBuf_MMPSize[j] = i;
			} else {
				pBuf_kmalloc[j] = kmalloc(
					(RT_BUF_TBL_NPAGES + 2) * PAGE_SIZE,
					GFP_KERNEL);

				if ((pBuf_kmalloc[j]) == NULL) {
					LOG_ERR("mem not enough\n");
					return -ENOMEM;
				}
				memset(pBuf_kmalloc[j], 0x00,
					(RT_BUF_TBL_NPAGES + 2) * PAGE_SIZE);
				Tbl_RTBuf_MMPSize[j] = (RT_BUF_TBL_NPAGES + 2);

			}
			/* round it up to the page bondary */
			pTbl_RTBuf[j] =
				(int *)((((unsigned long)pBuf_kmalloc[j]) +
				PAGE_SIZE - 1) & PAGE_MASK);

			pstRTBuf[j] = (struct ISP_RT_BUF_STRUCT *)pTbl_RTBuf[j];
			pstRTBuf[j]->state = ISP_RTBC_STATE_INIT;
			break;
		default:
			pBuf_kmalloc[j] = NULL;
			pTbl_RTBuf[j] = NULL;
			Tbl_RTBuf_MMPSize[j] = 0;
			break;
		}
	}


	/* isr log */
	if (PAGE_SIZE < ((ISP_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
		((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM)) {
		i = 0;
		while (i < ((ISP_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
			((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM))
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
		for (j = 0; j < ISP_IRQ_TYPE_AMOUNT; j++) {
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
	/* mark the pages reserved , FOR MMAP*/
	for (j = 0; j < ISP_IRQ_TYPE_AMOUNT; j++) {
		if (pTbl_RTBuf[j] != NULL) {
			for (i = 0; i < Tbl_RTBuf_MMPSize[j] * PAGE_SIZE;
				i += PAGE_SIZE)
				SetPageReserved(virt_to_page(
					((unsigned long)pTbl_RTBuf[j]) + i));

		}
	}

#ifndef EP_CODE_MARK_CMDQ
	/* Register ISP callback */
	LOG_DBG("register isp callback for MDP");
	cmdqCoreRegisterCB(CMDQ_GROUP_ISP,
			   ISP_MDPClockOnCallback,
			   ISP_MDPDumpCallback,
			   ISP_MDPResetCallback,
			   ISP_MDPClockOffCallback);
	/* Register GCE callback for dumping ISP register */
	LOG_DBG("register isp callback for GCE");
	cmdqCoreRegisterDebugRegDumpCB(ISP_BeginGCECallback,
		ISP_EndGCECallback);
#endif
	/* m4u_enable_tf(M4U_PORT_CAM_IMGI, 0);*/

#ifdef _MAGIC_NUM_ERR_HANDLING_
	LOG_DBG("init m_LastMNum");
	for (i = 0; i < _cam_max_; i++)
		m_LastMNum[i] = 0;

#endif


	for (i = 0; i < ISP_DEV_NODE_NUM; i++)
		SuspnedRecord[i] = 0;

	LOG_DBG("- X. Ret: %d.", Ret);
	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static void __exit ISP_Exit(void)
{
	int i, j;

	LOG_DBG("- E.");
	/*  */
	platform_driver_unregister(&IspDriver);
	/*  */
#ifndef EP_CODE_MARK_CMDQ
	/* Unregister ISP callback */
	cmdqCoreRegisterCB(CMDQ_GROUP_ISP,
			   NULL,
			   NULL,
			   NULL,
			   NULL);
	/* Un-Register GCE callback */
	LOG_DBG("Un-register isp callback for GCE");
	cmdqCoreRegisterDebugRegDumpCB(NULL, NULL);
#endif


	for (j = 0; j < ISP_IRQ_TYPE_AMOUNT; j++) {
		if (pTbl_RTBuf[j] != NULL) {
			/* unreserve the pages */
			for (i = 0; i < Tbl_RTBuf_MMPSize[j] * PAGE_SIZE;
				i += PAGE_SIZE)
				ClearPageReserved(virt_to_page(
					((unsigned long)pTbl_RTBuf[j]) + i));

			/* free the memory areas */
			kfree(pBuf_kmalloc[j]);
		}
	}

	/* free the memory areas */
	kfree(pLog_kmalloc);

	/*  */
}

int32_t ISP_MDPClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("ISP_MDPClockOnCallback"); */
	/*LOG_DBG("+MDPEn:%d", G_u4EnableClockCount);*/
	ISP_EnableClock(MTRUE);

	return 0;
}

int32_t ISP_MDPDumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("%s", __func__);

	ISP_DumpDIPReg();

	return 0;
}
int32_t ISP_MDPResetCallback(uint64_t engineFlag)
{
	LOG_DBG("%s", __func__);

	ISP_Reset(ISP_REG_SW_CTL_RST_CAM_P2);

	return 0;
}

int32_t ISP_MDPClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("ISP_MDPClockOffCallback"); */
	ISP_EnableClock(MFALSE);
	/*LOG_DBG("-MDPEn:%d", G_u4EnableClockCount);*/
	return 0;
}


#define ISP_IMGSYS_BASE_PHY_KK 0x15022000

static uint32_t addressToDump[] = {
#if 1
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x000),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x004),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x008),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x00C),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x010),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x014),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x018),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x204),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x208),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x20C),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x400),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x408),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x410),
(uint32_t)(ISP_IMGSYS_BASE_PHY_KK + 0x414),
#endif
};

int32_t ISP_BeginGCECallback(uint32_t taskID, uint32_t *regCount,
	uint32_t **regAddress)
{
	LOG_DBG("+,taskID(%d)", taskID);

	*regCount = sizeof(addressToDump) / sizeof(uint32_t);
	*regAddress = (uint32_t *)addressToDump;

	LOG_DBG("-,*regCount(%d)", *regCount);

	return 0;
}

int32_t ISP_EndGCECallback(uint32_t taskID, uint32_t regCount,
	uint32_t *regValues)
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
				add[j] = addressToDump[pos];
				val[j] = regValues[pos];
			}
		}
#define TMP_STR1 "[0x%08x,0x%08x][0x%08x,0x%08x][0x%08x,0x%08x]"
#define TMP_STR2 "[0x%08x,0x%08x][0x%08x,0x%08x]\n"

		LOG_DBG(TMP_STR1 TMP_STR2,
			add[0], val[0], add[1], val[1], add[2], val[2],
			add[3], val[3], add[4], val[4]);
		LOG_DBG(TMP_STR1 TMP_STR2,
			add[5], val[5], add[6], val[6], add[7], val[7],
			add[8], val[8], add[9], val[9]);
#undef TMP_STR1
#undef TMP_STR2
	}

#if DUMP_GCE_TPIPE
	/* tpipePA = ISP_RD32(ISP_IMGSYS_BASE_PHY_KK + 0x204); */
	tpipePA = val[7];
	/* ctlStart = ISP_RD32(ISP_IMGSYS_BASE_PHY_KK + 0x000); */
	ctlStart = val[0];

	LOG_DBG("kk:tpipePA(0x%x), ctlStart(0x%x)", tpipePA, ctlStart);

	if ((tpipePA)) {
#ifdef AEE_DUMP_BY_USING_ION_MEMORY
		tpipePA = tpipePA&0xfffff000;
		struct isp_imem_memory isp_p2GCEdump_imem_buf;

		struct ion_client *isp_p2GCEdump_ion_client;

		isp_p2GCEdump_imem_buf.handle = NULL;
		isp_p2GCEdump_imem_buf.ion_fd = 0;
		isp_p2GCEdump_imem_buf.va = 0;
		isp_p2GCEdump_imem_buf.pa = 0;
		isp_p2GCEdump_imem_buf.length = TPIPE_DUMP_SIZE;
		if ((isp_p2_ion_client == NULL) && (g_ion_device))
			isp_p2_ion_client =
				ion_client_create(g_ion_device, "isp_p2");
		if (isp_p2_ion_client == NULL)
			LOG_ERR("invalid isp_p2_ion_client client!\n");
		if (isp_allocbuf(&isp_p2GCEdump_imem_buf) >= 0) {
			pMapVa = (int *)isp_p2GCEdump_imem_buf.va;
		LOG_DBG("ctlStart(0x%x),tpipePA(0x%x)", ctlStart, tpipePA);

		if (pMapVa) {
			for (i = 0; i < TPIPE_DUMP_SIZE; i += 10) {
#define TMP_STR "[idx(%d)]%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X"
				LOG_DBG(TMP_STR, i,
					pMapVa[i], pMapVa[i+1], pMapVa[i+2],
					pMapVa[i+3], pMapVa[i+4], pMapVa[i+5],
					pMapVa[i+6], pMapVa[i+7], pMapVa[i+8],
					pMapVa[i+9]);
#undef TMP_STR
			}
		}
			isp_freebuf(&isp_p2GCEdump_imem_buf);
			isp_p2GCEdump_imem_buf.handle = NULL;
			isp_p2GCEdump_imem_buf.ion_fd = 0;
			isp_p2GCEdump_imem_buf.va = 0;
			isp_p2GCEdump_imem_buf.pa = 0;
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

static int DMAErrHandler(void)
{
	unsigned int err_ctrl = ISP_RD32(0xF50043A4);

	LOG_DBG("err_ctrl(0x%08x)", err_ctrl);

	unsigned int i = 0;

	unsigned int *pErr = DMA_ERR;

	for (i = 0; i < 12; i++) {
		unsigned int addr = 0;

#if 1
		if (err_ctrl & (*pErr)) {
			ISP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, ISP_RD32(addr),
				ISP_RD32(0xF5004160), ISP_RD32(0xF5004164));
		}
#else
		addr = pErr[1];
		unsigned int status = ISP_RD32(addr);

		if (status & 0x0000FFFF) {
			ISP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, status,
				ISP_RD32(0xF5004160), ISP_RD32(0xF5004164));
		}
#endif
		pErr = pErr + 3;
	}

}
#endif


void IRQ_INT_ERR_CHECK_CAM(unsigned int WarnStatus, unsigned int ErrStatus,
	enum ISP_IRQ_TYPE_ENUM module)
{
	/* ERR print */
	/* unsigned int i = 0; */
	if (ErrStatus) {
		switch (module) {
		case ISP_IRQ_TYPE_INT_CAM_A_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAM_A_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAM_A:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);

			/* DMA ERR print */
			if (ErrStatus & DMA_ERR_ST)
				ISP_DumpDmaDeepDbg(module);

			break;
		case ISP_IRQ_TYPE_INT_CAM_B_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAM_B_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAM_B:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);

			/* DMA ERR print */
			if (ErrStatus & DMA_ERR_ST)
				ISP_DumpDmaDeepDbg(module);

			break;
		case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAMSV_0_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAMSV0:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAMSV_1_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAMSV1:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAMSV_2_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAMSV2:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAMSV_3_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAMSV3:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAMSV_4_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAMSV4:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
			g_ISPIntErr[ISP_IRQ_TYPE_INT_CAMSV_5_ST] |=
				(ErrStatus|WarnStatus);
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"CAMSV5:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		case ISP_IRQ_TYPE_INT_DIP_A_ST:
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"DIP_A:int_err:0x%x_0x%x\n",
				WarnStatus, ErrStatus);
			break;
		default:
			break;
		}
		/*SMI monitor*/
/* ALSK TBD */
		/*
		 * if (WarnStatus & (RRZO_ERR_ST|AFO_ERR_ST|IMGO_ERR_ST|
		 * AAO_ERR_ST|LCSO_ERR_ST|BNR_ERR_ST|LSC_ERR_ST)) {
		 *	for (i = 0 ; i < 5 ; i++)
		 *		smi_dumpDebugMsg();
		 * }
		 */
	}
}


enum CAM_FrameST Irq_CAM_FrameStatus(enum ISP_DEV_NODE_ENUM module,
	enum ISP_IRQ_TYPE_ENUM irq_mod, unsigned int delayCheck)
{
	int dma_arry_map[_cam_max_] = {
		/*      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,*/
		 0, /* _imgo_*/
		 1, /* _rrzo_ */
		 2, /* _ufeo_ */
		-1, /* _aao_ */
		-1, /* _afo_ */
		 3, /* _lcso_ */
		-1, /* _pdo_ */
		 4, /* _eiso_ */
		-1, /* _flko_ */
		 5, /* _rsso_ */
		-1  /* _pso_ */
	};

	unsigned int dma_en;
	unsigned int uni_dma_en;
	union FBC_CTRL_1 fbc_ctrl1[6];
	union FBC_CTRL_2 fbc_ctrl2[6];
	unsigned int hds2_sel =
		(ISP_RD32(CAM_UNI_REG_TOP_PATH_SEL(ISP_UNI_A_IDX)) & 0x3);
	bool bQueMode = MFALSE;
	unsigned int product = 1;

	unsigned int i;

	if ((module != ISP_CAM_A_IDX) && (module != ISP_CAM_B_IDX)) {
		LOG_ERR("unsupported module:0x%x\n", module);
		return CAM_FST_DROP_FRAME;
	}

	dma_en = ISP_RD32(CAM_REG_CTL_DMA_EN(module));

	if (dma_en & 0x1) {
		fbc_ctrl1[dma_arry_map[_imgo_]].Raw =
			ISP_RD32(CAM_REG_FBC_IMGO_CTL1(module));
		fbc_ctrl2[dma_arry_map[_imgo_]].Raw =
			ISP_RD32(CAM_REG_FBC_IMGO_CTL2(module));
	} else {
		fbc_ctrl1[dma_arry_map[_imgo_]].Raw = 0x0;
		fbc_ctrl2[dma_arry_map[_imgo_]].Raw = 0x0;
	}

	if (dma_en & 0x2) {
		fbc_ctrl1[dma_arry_map[_ufeo_]].Raw =
			ISP_RD32(CAM_REG_FBC_UFEO_CTL1(module));
		fbc_ctrl2[dma_arry_map[_ufeo_]].Raw =
			ISP_RD32(CAM_REG_FBC_UFEO_CTL2(module));
	} else {
		fbc_ctrl1[dma_arry_map[_ufeo_]].Raw = 0x0;
		fbc_ctrl2[dma_arry_map[_ufeo_]].Raw = 0x0;
	}


	if (dma_en & 0x4) {
		fbc_ctrl1[dma_arry_map[_rrzo_]].Raw =
			ISP_RD32(CAM_REG_FBC_RRZO_CTL1(module));
		fbc_ctrl2[dma_arry_map[_rrzo_]].Raw =
			ISP_RD32(CAM_REG_FBC_RRZO_CTL2(module));
	} else {
		fbc_ctrl1[dma_arry_map[_rrzo_]].Raw = 0x0;
		fbc_ctrl2[dma_arry_map[_rrzo_]].Raw = 0x0;
	}

	if (dma_en & 0x10) {
		fbc_ctrl1[dma_arry_map[_lcso_]].Raw =
			ISP_RD32(CAM_REG_FBC_LCSO_CTL1(module));
		fbc_ctrl2[dma_arry_map[_lcso_]].Raw =
			ISP_RD32(CAM_REG_FBC_LCSO_CTL2(module));
	} else {
		fbc_ctrl1[dma_arry_map[_lcso_]].Raw = 0x0;
		fbc_ctrl2[dma_arry_map[_lcso_]].Raw = 0x0;
	}

	if (((hds2_sel == 0) && (module == ISP_CAM_A_IDX)) ||
		((hds2_sel == 1) && (module == ISP_CAM_B_IDX))) {
		uni_dma_en = ISP_RD32(CAM_UNI_REG_TOP_DMA_EN(ISP_UNI_A_IDX));

		if (uni_dma_en & 0x4) {
			fbc_ctrl1[dma_arry_map[_eiso_]].Raw =
				ISP_RD32(CAM_UNI_REG_FBC_EISO_A_CTL1(
					ISP_UNI_A_IDX));
			fbc_ctrl2[dma_arry_map[_eiso_]].Raw =
				ISP_RD32(CAM_UNI_REG_FBC_EISO_A_CTL2(
					ISP_UNI_A_IDX));
		} else {
			fbc_ctrl1[dma_arry_map[_eiso_]].Raw = 0x0;
			fbc_ctrl2[dma_arry_map[_eiso_]].Raw = 0x0;
		}

		if (uni_dma_en & 0x8) {
			fbc_ctrl1[dma_arry_map[_rsso_]].Raw =
				ISP_RD32(CAM_UNI_REG_FBC_RSSO_A_CTL1(
					ISP_UNI_A_IDX));
			fbc_ctrl2[dma_arry_map[_rsso_]].Raw =
				ISP_RD32(CAM_UNI_REG_FBC_RSSO_A_CTL2(
					ISP_UNI_A_IDX));
		} else {
			fbc_ctrl1[dma_arry_map[_rsso_]].Raw = 0x0;
			fbc_ctrl2[dma_arry_map[_rsso_]].Raw = 0x0;
		}
	} else {
		fbc_ctrl1[dma_arry_map[_eiso_]].Raw = 0x0;
		fbc_ctrl2[dma_arry_map[_eiso_]].Raw = 0x0;
		fbc_ctrl1[dma_arry_map[_rsso_]].Raw = 0x0;
		fbc_ctrl2[dma_arry_map[_rsso_]].Raw = 0x0;
	}

	for (i = 0; i < _cam_max_; i++) {
		if (dma_arry_map[i] >= 0) {
			if (fbc_ctrl1[dma_arry_map[i]].Raw != 0) {
				bQueMode = fbc_ctrl1[
					dma_arry_map[i]].Bits.FBC_MODE;
				break;
			}
		}
	}

	if (bQueMode) {
		for (i = 0; i < _cam_max_; i++) {
			if (dma_arry_map[i] < 0)
				continue;

			if (fbc_ctrl1[dma_arry_map[i]].Raw != 0) {
				product *= fbc_ctrl2[
					dma_arry_map[i]].Bits.FBC_CNT;
				if (product == 0)
					return CAM_FST_DROP_FRAME;
			}

		}
	} else {
		for (i = 0; i < _cam_max_; i++) {
			if (dma_arry_map[i] < 0)
				continue;

			if (fbc_ctrl1[dma_arry_map[i]].Raw != 0) {
				product *= (
					fbc_ctrl1[
						dma_arry_map[i]].Bits.FBC_NUM -
					fbc_ctrl2[
						dma_arry_map[i]].Bits.FBC_CNT);
				if (product == 0)
					return CAM_FST_DROP_FRAME;
			}

		}
	}

	if (product == 1)
		return CAM_FST_LAST_WORKING_FRAME;
	else
		return CAM_FST_NORMAL;
}

#if (TIMESTAMP_QUEUE_EN == 1)
static void ISP_GetDmaPortsStatus(enum ISP_DEV_NODE_ENUM reg_module,
	unsigned int *DmaPortsStats)
{
	unsigned int dma_en = ISP_RD32(CAM_REG_CTL_DMA_EN(reg_module));
	unsigned int hds2_sel =
		(ISP_RD32(CAM_UNI_REG_TOP_PATH_SEL(ISP_UNI_A_IDX)) & 0x3);
	unsigned int flk2_sel =
		((ISP_RD32(CAM_UNI_REG_TOP_PATH_SEL(ISP_UNI_A_IDX)) >> 8) &
		0x3);
	unsigned int uni_dma_en = 0;

	DmaPortsStats[_imgo_] = ((dma_en & 0x01) ? 1 : 0);
	DmaPortsStats[_ufeo_] = ((dma_en & 0x02) ? 1 : 0);
	DmaPortsStats[_rrzo_] = ((dma_en & 0x04) ? 1 : 0);
	DmaPortsStats[_lcso_] = ((dma_en & 0x10) ? 1 : 0);

	uni_dma_en = 0;
	if (((hds2_sel == 0) && (reg_module == ISP_CAM_A_IDX)) ||
		((hds2_sel == 1) && (reg_module == ISP_CAM_B_IDX)))
		uni_dma_en = ISP_RD32(CAM_UNI_REG_TOP_DMA_EN(ISP_UNI_A_IDX));

	DmaPortsStats[_eiso_] = ((uni_dma_en & 0x04) ? 1 : 0);
	DmaPortsStats[_rsso_] = ((uni_dma_en & 0x08) ? 1 : 0);

	DmaPortsStats[_aao_] = ((dma_en & 0x20) ? 1 : 0);
	DmaPortsStats[_pso_] = ((dma_en & 0x40) ? 1 : 0);
	DmaPortsStats[_afo_] = ((dma_en & 0x08) ? 1 : 0);
	DmaPortsStats[_pdo_] = ((dma_en & 0x400) ? 1 : 0);


	uni_dma_en = 0;
	if (((flk2_sel == 0) && (reg_module == ISP_CAM_A_IDX)) ||
		((flk2_sel == 1) && (reg_module == ISP_CAM_B_IDX)))
		uni_dma_en = ISP_RD32(CAM_UNI_REG_TOP_DMA_EN(ISP_UNI_A_IDX));

	DmaPortsStats[_flko_] = ((uni_dma_en & 0x02) ? 1 : 0);

}

static enum CAM_FrameST Irq_CAM_SttFrameStatus(enum ISP_DEV_NODE_ENUM module,
	enum ISP_IRQ_TYPE_ENUM irq_mod, unsigned int dma_id,
	unsigned int delayCheck)
{
	static const int dma_arry_map[_cam_max_] = {
		-1, /* _imgo_*/
		-1, /* _rrzo_ */
		-1, /* _ufeo_ */
		 0, /* _aao_ */
		 1, /* _afo_ */
		-1, /* _lcso_ */
		 2, /* _pdo_ */
		-1, /* _eiso_ */
		 3, /* _flko_ */
		-1, /* _rsso_ */
		 4  /* _pso_ */
	};

	unsigned int     dma_en;
	unsigned int     uni_dma_en;
	union FBC_CTRL_1  fbc_ctrl1;
	union FBC_CTRL_2  fbc_ctrl2;
	unsigned int     flk2_sel =
		((ISP_RD32(CAM_UNI_REG_TOP_PATH_SEL(ISP_UNI_A_IDX)) >> 8) &
		0x3);

	bool       bQueMode = MFALSE;
	unsigned int     product = 1;
	/* TSTP_V3 unsigned int     frmPeriod = 1; */

	switch (module) {
	case ISP_CAM_A_IDX:
	case ISP_CAM_B_IDX:
		if (dma_id >= _cam_max_) {
			LOG_ERR("LINE_%d ERR: unsupported module:0x%x dma:%d\n",
				__LINE__, module, dma_id);
			return CAM_FST_DROP_FRAME;
		}
		if (dma_arry_map[dma_id] < 0) {
			LOG_ERR("LINE_%d ERR: unsupported module:0x%x dma:%d\n",
				__LINE__, module, dma_id);
			return CAM_FST_DROP_FRAME;
		}
		break;
	default:
		LOG_ERR("LINE_%d ERROR: unsupported module:0x%x dma:%d\n",
			__LINE__, module, dma_id);
		return CAM_FST_DROP_FRAME;
	}

	fbc_ctrl1.Raw = 0x0;
	fbc_ctrl2.Raw = 0x0;

	dma_en = ISP_RD32(CAM_REG_CTL_DMA_EN(module));

	if (_aao_ == dma_id) {
		if (dma_en & 0x20) {
			fbc_ctrl1.Raw = ISP_RD32(CAM_REG_FBC_AAO_CTL1(module));
			fbc_ctrl2.Raw = ISP_RD32(CAM_REG_FBC_AAO_CTL2(module));
		}
	}

	if (_afo_ == dma_id) {
		if (dma_en & 0x8) {
			fbc_ctrl1.Raw = ISP_RD32(CAM_REG_FBC_AFO_CTL1(module));
			fbc_ctrl2.Raw = ISP_RD32(CAM_REG_FBC_AFO_CTL2(module));
		}
	}

	if (_pdo_ == dma_id) {
		if (dma_en & 0x400) {
			fbc_ctrl1.Raw = ISP_RD32(CAM_REG_FBC_PDO_CTL1(module));
			fbc_ctrl2.Raw = ISP_RD32(CAM_REG_FBC_PDO_CTL2(module));
		}
	}

	if (_flko_ == dma_id) {
		if (((flk2_sel == 0) && (module == ISP_CAM_A_IDX)) ||
			((flk2_sel == 1) && (module == ISP_CAM_B_IDX))) {
			uni_dma_en =
				ISP_RD32(CAM_UNI_REG_TOP_DMA_EN(ISP_UNI_A_IDX));

			if (uni_dma_en & 0x2) {
				fbc_ctrl1.Raw =
					ISP_RD32(CAM_UNI_REG_FBC_FLKO_A_CTL1(
						ISP_UNI_A_IDX));
				fbc_ctrl2.Raw =
					ISP_RD32(CAM_UNI_REG_FBC_FLKO_A_CTL2(
						ISP_UNI_A_IDX));
			}
		}
	}

	if (_pso_ == dma_id) {
		if (dma_en & 0x40) {
			fbc_ctrl1.Raw = ISP_RD32(CAM_REG_FBC_PSO_CTL1(module));
			fbc_ctrl2.Raw = ISP_RD32(CAM_REG_FBC_PSO_CTL2(module));
		}
	}

	bQueMode = fbc_ctrl1.Bits.FBC_MODE;

	if (bQueMode) {
		if (fbc_ctrl1.Raw != 0) {
			product *= fbc_ctrl2.Bits.FBC_CNT;

			if (product == 0)
				return CAM_FST_DROP_FRAME;
		} else
			return CAM_FST_DROP_FRAME;
	} else {
		if (fbc_ctrl1.Raw != 0) {
			product *= (fbc_ctrl1.Bits.FBC_NUM -
				fbc_ctrl2.Bits.FBC_CNT);

			if (product == 0)
				return CAM_FST_DROP_FRAME;
		} else
			return CAM_FST_DROP_FRAME;
	}

	if (product == 1)
		return CAM_FST_LAST_WORKING_FRAME;
	else
		return CAM_FST_NORMAL;

}

static int32_t ISP_PushBufTimestamp(unsigned int module, unsigned int dma_id,
	unsigned int sec, unsigned int usec, unsigned int frmPeriod)
{
	unsigned int wridx = 0;
	union FBC_CTRL_2 fbc_ctrl2;
	enum ISP_DEV_NODE_ENUM reg_module;

	fbc_ctrl2.Raw = 0x0;

	switch (module) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
		reg_module = ISP_CAM_A_IDX;
		break;
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		reg_module = ISP_CAM_B_IDX;
		break;
	default:
		LOG_ERR("Unsupport module:x%x\n", module);
		return -EFAULT;
	}

	switch (module) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		switch (dma_id) {
		case _imgo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_IMGO_CTL2(reg_module));
			break;
		case _rrzo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_RRZO_CTL2(reg_module));
			break;
		case _ufeo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_UFEO_CTL2(reg_module));
			break;
		case _eiso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_UNI_REG_FBC_EISO_A_CTL2(
					ISP_UNI_A_IDX));
			break;
		case _lcso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_LCSO_CTL2(reg_module));
			break;
		case _rsso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_UNI_REG_FBC_RSSO_A_CTL2(
					ISP_UNI_A_IDX));
			break;

		case _aao_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_AAO_CTL2(reg_module));
			break;
		case _afo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_AFO_CTL2(reg_module));
			break;
		case _flko_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_UNI_REG_FBC_FLKO_A_CTL2(
					ISP_UNI_A_IDX));
			break;
		case _pdo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_PDO_CTL2(reg_module));
			break;
		case _pso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_PSO_CTL2(reg_module));
			break;
		default:
			LOG_ERR("Unsupport dma:x%x\n", dma_id);
			return -EFAULT;
		}
		break;
	default:
		return -EFAULT;
	}

	if (frmPeriod > 1)
		fbc_ctrl2.Bits.WCNT =
			(fbc_ctrl2.Bits.WCNT / frmPeriod) * frmPeriod;

	if (((fbc_ctrl2.Bits.WCNT + frmPeriod) & 63) ==
		IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt) {
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
			"Cam:%d dma:%d ignore push wcnt_%d_%d\n",
			module, dma_id,
			IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt,
			fbc_ctrl2.Bits.WCNT);
		return 0;
	}

	wridx = IspInfo.TstpQInfo[module].Dmao[dma_id].WrIndex;

	IspInfo.TstpQInfo[module].Dmao[dma_id].TimeQue[wridx].sec = sec;
	IspInfo.TstpQInfo[module].Dmao[dma_id].TimeQue[wridx].usec = usec;

	if (IspInfo.TstpQInfo[module].Dmao[dma_id].WrIndex >=
		(ISP_TIMESTPQ_DEPTH-1))
		IspInfo.TstpQInfo[module].Dmao[dma_id].WrIndex = 0;
	else
		IspInfo.TstpQInfo[module].Dmao[dma_id].WrIndex++;

	IspInfo.TstpQInfo[module].Dmao[dma_id].TotalWrCnt++;

	/* Update WCNT for patch timestamp when SOF ISR missing */
	IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt =
		((IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt + 1) &
		0x3F);

	return 0;
}

static int32_t ISP_PopBufTimestamp(unsigned int module, unsigned int dma_id,
	struct S_START_T *pTstp)
{
	switch (module) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		switch (dma_id) {
		case _imgo_:
		case _rrzo_:
		case _ufeo_:
		case _eiso_:
		case _lcso_:

		case _flko_:
		case _aao_:
		case _afo_:
		case _rsso_:
		case _pdo_:
		case _pso_:
			break;
		default:
			LOG_ERR("Unsupport dma:x%x\n", dma_id);
			return -EFAULT;
		}
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
		switch (dma_id) {
		case _camsv_imgo_:
			break;
		default:
			LOG_ERR("Unsupport dma:x%x\n", dma_id);
			return -EFAULT;
		}
		break;
	default:
		LOG_ERR("Unsupport module:x%x\n", module);
		return -EFAULT;
	}

	if (pTstp)
		*pTstp = IspInfo.TstpQInfo[module].Dmao[dma_id].
			TimeQue[IspInfo.TstpQInfo[module].Dmao[dma_id].RdIndex];

	if (IspInfo.TstpQInfo[module].Dmao[dma_id].RdIndex >=
		(ISP_TIMESTPQ_DEPTH - 1))
		IspInfo.TstpQInfo[module].Dmao[dma_id].RdIndex = 0;
	else
		IspInfo.TstpQInfo[module].Dmao[dma_id].RdIndex++;

	IspInfo.TstpQInfo[module].Dmao[dma_id].TotalRdCnt++;

	return 0;
}


static int32_t ISP_WaitTimestampReady(unsigned int module, unsigned int dma_id)
{
	unsigned int _timeout = 0;
	unsigned int wait_cnt = 0;

	if (IspInfo.TstpQInfo[module].Dmao[dma_id].TotalWrCnt >
		IspInfo.TstpQInfo[module].Dmao[dma_id].TotalRdCnt)
		return 0;

	LOG_INF("Wait module:%d dma:%d timestamp ready W/R:%d/%d\n",
		module, dma_id,
		(unsigned int)
		  IspInfo.TstpQInfo[module].Dmao[dma_id].TotalWrCnt,
		(unsigned int)
		  IspInfo.TstpQInfo[module].Dmao[dma_id].TotalRdCnt);

	#if 1
	for (wait_cnt = 3; wait_cnt > 0; wait_cnt--) {
		_timeout = wait_event_interruptible_timeout(
			IspInfo.WaitQueueHead[module],
			(IspInfo.TstpQInfo[module].Dmao[dma_id].TotalWrCnt >
			  IspInfo.TstpQInfo[module].Dmao[dma_id].TotalRdCnt),
			ISP_MsToJiffies(2000));
		/* check if user is interrupted by system signal */
		if ((_timeout != 0) &&
			(!(IspInfo.TstpQInfo[module].Dmao[dma_id].TotalWrCnt >
			IspInfo.TstpQInfo[module].Dmao[dma_id].TotalRdCnt))) {
			LOG_INF("interrupted by system signal, ret val(%d)\n",
				_timeout);
			return -ERESTARTSYS;
		}

		if (_timeout > 0)
			break;

		LOG_INF("WARNING: cam:%d dma:%d wait left count %d\n",
			module, dma_id, wait_cnt);
	}
	if (wait_cnt == 0) {
		LOG_ERR("ERROR: cam:%d dma:%d wait timestamp timeout!!!\n",
			module, dma_id);
		return -EFAULT;
	}
	#else
	while (IspInfo.TstpQInfo[module].Dmao[dma_id].TotalWrCnt <=
		IspInfo.TstpQInfo[module].Dmao[dma_id].TotalRdCnt) {

		msleep(20);

		if ((wait_cnt & 0x7) == 0x7) {
#define TMP_STR "WARNING: module:%d dma:%d wait long %d W/R:%d/%d\n"
			LOG_INF(TMP_STR,
				module, dma_id, wait_cnt,
				(unsigned int)IspInfo.TstpQInfo[
					module].Dmao[dma_id].TotalWrCnt,
				(unsigned int)IspInfo.TstpQInfo[
					module].Dmao[dma_id].TotalRdCnt);
#undef TMP_STR
		}

		if (wait_cnt > 3000) {
#define ERR_STR "ERROR: module:%d dma:%d wait timeout %d W/R:%d/%d\n"
			LOG_INF(ERR_STR,
				module, dma_id, wait_cnt,
				(unsigned int)IspInfo.TstpQInfo[
					module].Dmao[dma_id].TotalWrCnt,
				(unsigned int)IspInfo.TstpQInfo[
					module].Dmao[dma_id].TotalRdCnt);
#undef ERR_STR
			break;
		}

		wait_cnt++;
	}
	#endif

	return 0;
}

static int32_t ISP_CompensateMissingSofTime(enum ISP_DEV_NODE_ENUM reg_module,
	unsigned int module, unsigned int dma_id, unsigned int sec,
	unsigned int usec, unsigned int frmPeriod)
{
	union FBC_CTRL_2  fbc_ctrl2;
	unsigned int delta_wcnt = 0, wridx = 0;
	unsigned int wridx_prev1 = 0, wridx_prev2 = 0, i = 0;
	unsigned int delta_time = 0, max_delta_time = 0;
	struct S_START_T   time_prev1, time_prev2;
	/*To shrink error log, only rrzo print error log*/
	bool dmao_mask = MFALSE;
	/*
	 * Patch timestamp and WCNT base on current HW WCNT and
	 * previous SW WCNT value, and calculate difference
	 */

	fbc_ctrl2.Raw = 0;

	switch (module) {
	case ISP_IRQ_TYPE_INT_CAM_A_ST:
	case ISP_IRQ_TYPE_INT_CAM_B_ST:
		switch (dma_id) {
		case _imgo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_IMGO_CTL2(reg_module));
			break;
		case _rrzo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_RRZO_CTL2(reg_module));
			dmao_mask = MTRUE;
			break;
		case _ufeo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_UFEO_CTL2(reg_module));
			break;
		case _eiso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_UNI_REG_FBC_EISO_A_CTL2(
					ISP_UNI_A_IDX));
			break;
		case _lcso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_LCSO_CTL2(reg_module));
			break;
		case _rsso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_UNI_REG_FBC_RSSO_A_CTL2(
					ISP_UNI_A_IDX));
			break;
		case _aao_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_AAO_CTL2(reg_module));
			break;
		case _afo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_AFO_CTL2(reg_module));
			break;
		case _flko_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_UNI_REG_FBC_FLKO_A_CTL2(
					ISP_UNI_A_IDX));
			break;
		case _pdo_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_PDO_CTL2(reg_module));
			break;
		case _pso_:
			fbc_ctrl2.Raw =
				ISP_RD32(CAM_REG_FBC_PSO_CTL2(reg_module));
			break;
		default:
			LOG_ERR("Unsupport dma:x%x\n", dma_id);
			return -EFAULT;
		}
		break;
	case ISP_IRQ_TYPE_INT_CAMSV_0_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_1_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_2_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_3_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_4_ST:
	case ISP_IRQ_TYPE_INT_CAMSV_5_ST:
	default:
		LOG_ERR("Unsupport module:x%x\n", module);
		return -EFAULT;
	}

	if (frmPeriod > 1)
		fbc_ctrl2.Bits.WCNT =
			(fbc_ctrl2.Bits.WCNT / frmPeriod) * frmPeriod;

	if (((fbc_ctrl2.Bits.WCNT + frmPeriod) & 63) ==
		IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt) {
		if (dmao_mask)
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"Cam:%d dma:%d ignore compensate wcnt_%d_%d\n",
				module, dma_id,
				IspInfo.TstpQInfo[module].Dmao[
					dma_id].PrevFbcWCnt,
				fbc_ctrl2.Bits.WCNT);
		return 0;
	}

	if (IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt >
		fbc_ctrl2.Bits.WCNT)
		delta_wcnt = fbc_ctrl2.Bits.WCNT + 64 -
			IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt;
	else
		delta_wcnt = fbc_ctrl2.Bits.WCNT -
			IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt;

	if (delta_wcnt > 255) {
		if (dmao_mask)
			LOG_ERR("ERROR: Cam:%d dma:%d WRONG WCNT:%d_%d_%d\n",
				module, dma_id, delta_wcnt,
				IspInfo.TstpQInfo[module].Dmao[
					dma_id].PrevFbcWCnt,
				fbc_ctrl2.Bits.WCNT);

		return -EFAULT;
	} else if (delta_wcnt > 6) {
		if (dmao_mask) {
#define ERR_STR "WARNING: Cam:%d dma:%d SUSPICIOUS WCNT:%d_%d_%d\n"
			LOG_ERR(ERR_STR,
				module, dma_id, delta_wcnt,
				IspInfo.TstpQInfo[module].Dmao[
					dma_id].PrevFbcWCnt,
				fbc_ctrl2.Bits.WCNT);
#undef ERR_STR
		}
	} else if (delta_wcnt == 0) {
		return 0;
	}

	/* delta_wcnt *= frmPeriod; */

	/* Patch missing SOF timestamp */
	wridx = IspInfo.TstpQInfo[module].Dmao[dma_id].WrIndex;
	wridx_prev1 = (wridx == 0) ? (ISP_TIMESTPQ_DEPTH - 1) : (wridx - 1);
	wridx_prev2 = (wridx_prev1 == 0) ?
		(ISP_TIMESTPQ_DEPTH - 1) : (wridx_prev1 - 1);

	time_prev1.sec = IspInfo.TstpQInfo[
		module].Dmao[dma_id].TimeQue[wridx_prev1].sec;

	time_prev1.usec = IspInfo.TstpQInfo[
		module].Dmao[dma_id].TimeQue[wridx_prev1].usec;

	time_prev2.sec = IspInfo.TstpQInfo[
		module].Dmao[dma_id].TimeQue[wridx_prev2].sec;

	time_prev2.usec = IspInfo.TstpQInfo[
		module].Dmao[dma_id].TimeQue[wridx_prev2].usec;

	if ((sec > time_prev1.sec) ||
		((sec == time_prev1.sec) && (usec > time_prev1.usec))) {
		max_delta_time =
			((sec - time_prev1.sec)*1000000 + usec) -
			time_prev1.usec;
	} else {
		if (dmao_mask) {
#define ERR_STR1 "ERROR: Cam:%d dma:%d current timestamp:"
#define ERR_STR2 " cur: %d.%06d prev1: %d.%06d\n"
			LOG_ERR(ERR_STR1 ERR_STR2,
				module, dma_id, sec, usec, time_prev1.sec,
				time_prev1.usec);
#undef ERR_STR1
#undef ERR_STR2
		}
		max_delta_time = 0;
	}

	if ((time_prev1.sec > time_prev2.sec) ||
		((time_prev1.sec == time_prev2.sec) &&
		(time_prev1.usec > time_prev2.usec)))
		delta_time =
			((time_prev1.sec - time_prev2.sec) * 1000000 +
			time_prev1.usec) - time_prev2.usec;
	else {
		if (dmao_mask) {
#define ERR_STR1 "ERROR: Cam:%d dma:%d previous timestamp:"
#define ERR_STR2 " prev1: %d.%06d prev2: %d.%06d\n"
			LOG_ERR(ERR_STR1 ERR_STR2,
				module, dma_id, time_prev1.sec, time_prev1.usec,
				time_prev2.sec, time_prev2.usec);
#undef ERR_STR1
#undef ERR_STR2
		}
		delta_time = 0;
	}

	if (delta_time > (max_delta_time / delta_wcnt)) {
		if (dmao_mask) {
#define TMP_STR1 "WARNING: Cam:%d dma:%d delta time too large:"
#define TMP_STR2 " cur %dus max %dus patch wcnt: %d\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				module, dma_id, delta_time, max_delta_time,
				delta_wcnt);
		}
		delta_time = max_delta_time / delta_wcnt;
	}

	for (i = 0; i < delta_wcnt; i++) {
		time_prev1.usec += delta_time;
		while (time_prev1.usec >= 1000000) {
			time_prev1.usec -= 1000000;
			time_prev1.sec++;
		}
		/* WCNT will be increase in this API */
		ISP_PushBufTimestamp(module, dma_id, time_prev1.sec,
			time_prev1.usec, frmPeriod);
	}

	if (dmao_mask)
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
			"Cam:%d dma:%d wcnt:%d_%d_%d T:%d.%06d_.%06d_%d.%06d\n",
			module, dma_id, delta_wcnt,
			IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt,
			fbc_ctrl2.Bits.WCNT, sec, usec, delta_time,
			time_prev1.sec, time_prev1.usec);

	if (IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt !=
		fbc_ctrl2.Bits.WCNT) {
		if (dmao_mask) {
#define ERR_STR "ERROR: Cam:%d dma:%d strange WCNT SW_HW: %d_%d\n"
			LOG_ERR(ERR_STR,
				module, dma_id,
				IspInfo.TstpQInfo[module].Dmao[
					dma_id].PrevFbcWCnt,
				fbc_ctrl2.Bits.WCNT);
#undef ERR_STR
		}
		IspInfo.TstpQInfo[module].Dmao[dma_id].PrevFbcWCnt =
			fbc_ctrl2.Bits.WCNT;
	}

	return 0;
}

#if (TSTMP_SUBSAMPLE_INTPL == 1)
static int32_t ISP_PatchTimestamp(unsigned int module, unsigned int dma_id,
	unsigned int frmPeriod, unsigned long long refTimestp,
	unsigned long long prevTimestp)
{
	unsigned long long prev_tstp = prevTimestp, cur_tstp = refTimestp;
	unsigned int target_wridx = 0, curr_wridx = 0;
	unsigned int frm_dt = 0, last_frm_dt = 0, i = 1;

	/* Only sub-sample case needs patch */
	if (frmPeriod <= 1)
		return 0;

	curr_wridx = IspInfo.TstpQInfo[module].Dmao[dma_id].WrIndex;

	if (curr_wridx < frmPeriod)
		target_wridx = (curr_wridx + ISP_TIMESTPQ_DEPTH - frmPeriod);
	else
		target_wridx = curr_wridx - frmPeriod;

	frm_dt = (((unsigned int)(cur_tstp - prev_tstp)) / frmPeriod);
	last_frm_dt = ((cur_tstp - prev_tstp) - frm_dt*(frmPeriod-1));

	if (frm_dt == 0)
		LOG_INF("WARNING: timestamp delta too small: %d\n",
			(int)(cur_tstp - prev_tstp));

	i = 0;
	while (target_wridx != curr_wridx) {

		if (i > frmPeriod) {
#define ERR_STR "Error: too many intpl in sub-sample period %d_%d\n"
			LOG_ERR(ERR_STR, target_wridx, curr_wridx);
#undef ERR_STR
			return -EFAULT;
		}

		IspInfo.TstpQInfo[module].Dmao[
			dma_id].TimeQue[target_wridx].usec += (frm_dt * i);

		while (IspInfo.TstpQInfo[module].Dmao[
			dma_id].TimeQue[target_wridx].usec >= 1000000) {

			IspInfo.TstpQInfo[module].Dmao[
				dma_id].TimeQue[target_wridx].usec -= 1000000;
			IspInfo.TstpQInfo[module].Dmao[
				dma_id].TimeQue[target_wridx].sec++;
		}

		i++;
		target_wridx++; /* patch from 2nd time */
		if (target_wridx >= ISP_TIMESTPQ_DEPTH)
			target_wridx = 0;
	}

	return 0;
}
#endif

#endif

irqreturn_t ISP_Irq_DIP_A(int  Irq, void *DeviceId)
{
	int i = 0;
	unsigned int IrqINTStatus = 0x0;
	unsigned int IrqCQStatus = 0x0;
	unsigned int IrqCQLDStatus = 0x0;

	/*LOG_DBG("ISP_Irq_DIP_A:%d\n", Irq);*/

	/* Avoid to touch hw module when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	spin_lock(&(IspInfo.SpinLockIrq[ISP_IRQ_TYPE_INT_DIP_A_ST]));
	/* DIP_A_REG_CTL_INT_STATUS */
	IrqINTStatus = ISP_RD32(ISP_DIP_A_BASE + 0x030);
	/* DIP_A_REG_CTL_CQ_INT_STATUS */
	IrqCQStatus = ISP_RD32(ISP_DIP_A_BASE + 0x034);
	IrqCQLDStatus = ISP_RD32(ISP_DIP_A_BASE + 0x038);

	for (i = 0; i < IRQ_USER_NUM_MAX; i++)
		IspInfo.IrqInfo.Status[ISP_IRQ_TYPE_INT_DIP_A_ST][SIGNAL_INT][
			i] |= IrqINTStatus;

	spin_unlock(&(IspInfo.SpinLockIrq[ISP_IRQ_TYPE_INT_DIP_A_ST]));

	/*LOG_DBG("ISP_Irq_DIP_A:%d, reg 0x%p : 0x%x, reg 0x%p : 0x%x\n",
	 *	Irq, (ISP_DIP_A_BASE + 0x030), IrqINTStatus,
	 * (ISP_DIP_A_BASE + 0x034), IrqCQStatus);
	 */

	/*  */
	wake_up_interruptible(
		&IspInfo.WaitQueueHead[ISP_IRQ_TYPE_INT_DIP_A_ST]);

	return IRQ_HANDLED;

}

irqreturn_t ISP_Irq_CAMSV_0(int  Irq, void *DeviceId)
{
	/* LOG_DBG("ISP_IRQ_CAM_SV:0x%x\n",Irq); */
	unsigned int module = ISP_IRQ_TYPE_INT_CAMSV_0_ST;
	unsigned int reg_module = ISP_CAMSV0_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;

	union FBC_CTRL_1 fbc_ctrl1[2];
	/*  */
	union FBC_CTRL_2 fbc_ctrl2[2];
	/*  */
	unsigned int time_stamp;
	/*  */
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;

	/* Avoid to touch hw module when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*  */
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAMSV_REG_INT_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];

	/*  */
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module));
	time_stamp       = ISP_RD32(CAMSV_REG_TG_TIME_STAMP(reg_module));

	/* sof , done order chech . */
	if ((IrqStatus & SV_HW_PASS1_DON_ST) || (IrqStatus & SV_SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & SV_HW_PASS1_DON_ST) && (IrqStatus & SV_SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & SV_SW_PASS1_DON_ST) {
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */
		/* update pass1 done time stamp for eis user(need to match with
		 * the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
#define TMP_STR "CAMSV0 P1_DON_%d(0x%08x_0x%08x) stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR,
				(sof_count[module]) ?
					(sof_count[module] - 1) :
					(sof_count[module]),
				(unsigned int)(fbc_ctrl1[0].Raw),
				(unsigned int)(fbc_ctrl2[0].Raw),
				time_stamp);
#undef TMP_STR
		}

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
			pstRTBuf[module]->ring_buf[_camsv_imgo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SV_SOF_INT_ST) {
		time = ktime_get();     /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

#if 0 /* always return CAM_FST_DROP_FRAME for CAMSV0~CAMSV5 */
		/* chk this frame have EOF or not, dynimic dma port chk */
		/* ISP_Irq_CAMSV_0 */
		FrameStatus[module] = Irq_CAM_FrameStatus(reg_module);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMSV0 Lost p1 done_%d (0x%08x): ",
				       sof_count[module], cur_v_cnt);
		}
#endif

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
#if 0
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
			else
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
#endif
			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
#define TMP_STR1 "CAMSV0 P1_SOF_%d_%d(0x%08x_0x%08x,0x%08x),"
#define TMP_STR2 "int_us:0x%08x, stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module))),
				ISP_RD32(CAMSV_REG_IMGO_BASE_ADDR(reg_module)),
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				time_stamp);
#undef TMP_STR1
#undef TMP_STR2

			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				((ISP_RD32(CAMSV_REG_TG_INTER_ST(reg_module)) &
				0x00FF0000) >> 16))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need to match with the
		 * time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		/* sw sof counter */
		sof_count[module]++;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;
					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;
					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this irq
		 * type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SV_SOF_INT_ST | SV_SW_PASS1_DON_ST | SV_VS1_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;

}

irqreturn_t ISP_Irq_CAMSV_1(int  Irq, void *DeviceId)
{
	/* LOG_DBG("ISP_IRQ_CAM_SV:0x%x\n",Irq); */
	unsigned int module = ISP_IRQ_TYPE_INT_CAMSV_1_ST;
	unsigned int reg_module = ISP_CAMSV1_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	/* */
	union FBC_CTRL_2 fbc_ctrl2[2];
	/* */
	unsigned int time_stamp;
	/* */
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;

	/* Avoid to touch hw module when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*  */
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAMSV_REG_INT_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];
	/*  */
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module));
	time_stamp       = ISP_RD32(CAMSV_REG_TG_TIME_STAMP(reg_module));

	/* sof , done order chech . */
	if ((IrqStatus & SV_HW_PASS1_DON_ST) || (IrqStatus & SV_SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & SV_HW_PASS1_DON_ST) && (IrqStatus & SV_SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & SV_SW_PASS1_DON_ST) {
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */
		/* update pass1 done time stamp for eis user(need to match with
		 * the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);
#if 0
		if (IspInfo.DebugMask & ISP_DBG_INT) {
#define TMP_STR "CAMSV1 P1_DON_%d(0x%08x_0x%08x) stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR,
				(sof_count[module]) ?
					(sof_count[module] - 1) :
					(sof_count[module]),
				(unsigned int)(fbc_ctrl1[0].Raw),
				(unsigned int)(fbc_ctrl2[0].Raw),
				time_stamp);
#undef TMP_STR
		}
#endif
		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
			pstRTBuf[module]->ring_buf[_camsv_imgo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SV_SOF_INT_ST) {
		time = ktime_get();     /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

#if 0 /* always return CAM_FST_DROP_FRAME for CAMSV0~CAMSV5 */
		/* chk this frame have EOF or not, dynimic dma port chk */
		/* ISP_Irq_CAMSV_1 */
		FrameStatus[module] = Irq_CAM_FrameStatus(reg_module);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMSV1 Lost p1 done_%d (0x%08x): ",
				       sof_count[module], cur_v_cnt);
		}
#endif

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
#if 0
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
			else
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
#endif
			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
#if 0
#define TMP_STR1 "CAMSV1 P1_SOF_%d_%d(0x%08x_0x%08x,0x%08x),"
#define TMP_STR2 "int_us:0x%08x, stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module))),
				ISP_RD32(CAMSV_REG_IMGO_BASE_ADDR(reg_module)),
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				time_stamp);
#undef TMP_STR1
#undef TMP_STR2
#endif
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				((ISP_RD32(CAMSV_REG_TG_INTER_ST(reg_module)) &
				0x00FF0000) >> 16))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need to match with the
		 * time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		/* sw sof counter */
		sof_count[module]++;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;
					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;
					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this irq
		 * type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SV_SOF_INT_ST | SV_SW_PASS1_DON_ST | SV_VS1_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;

}

irqreturn_t ISP_Irq_CAMSV_2(int  Irq, void *DeviceId)
{
	/* LOG_DBG("ISP_IRQ_CAM_SV:0x%x\n",Irq); */
	unsigned int module = ISP_IRQ_TYPE_INT_CAMSV_2_ST;
	unsigned int reg_module = ISP_CAMSV2_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	/* */
	union FBC_CTRL_2 fbc_ctrl2[2];
	/* */
	unsigned int time_stamp;
	/* */
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;

	/* Avoid to touch hw module when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*  */
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAMSV_REG_INT_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];
	/*  */
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module));
	time_stamp       = ISP_RD32(CAMSV_REG_TG_TIME_STAMP(reg_module));

	/* sof , done order chech . */
	if ((IrqStatus & SV_HW_PASS1_DON_ST) || (IrqStatus & SV_SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & SV_HW_PASS1_DON_ST) && (IrqStatus & SV_SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & SV_SW_PASS1_DON_ST) {
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */
		/* update pass1 done time stamp for eis user(need match with
		 * the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
#define TMP_STR "CAMSV2 P1_DON_%d(0x%08x_0x%08x) stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR,
				(sof_count[module]) ?
					(sof_count[module] - 1) :
					(sof_count[module]),
				(unsigned int)(fbc_ctrl1[0].Raw),
				(unsigned int)(fbc_ctrl2[0].Raw),
				time_stamp);
#undef TMP_STR
		}

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
			pstRTBuf[module]->ring_buf[_camsv_imgo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SV_SOF_INT_ST) {
		time = ktime_get();     /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

#if 0 /* always return CAM_FST_DROP_FRAME for CAMSV0~CAMSV5 */
		/* chk this frame have EOF or not, dynimic dma port chk */
		/* ISP_Irq_CAMSV_2 */
		FrameStatus[module] = Irq_CAM_FrameStatus(reg_module);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMSV2 Lost p1 done_%d (0x%08x): ",
				       sof_count[module], cur_v_cnt);
		}
#endif


		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
#if 0
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
			else
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
#endif
			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
#define TMP_STR1 "CAMSV2 P1_SOF_%d_%d(0x%08x_0x%08x,0x%08x),"
#define TMP_STR2 "int_us:0x%08x, stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module))),
				ISP_RD32(CAMSV_REG_IMGO_BASE_ADDR(reg_module)),
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				time_stamp);
#undef TMP_STR1
#undef TMP_STR2
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				((ISP_RD32(CAMSV_REG_TG_INTER_ST(reg_module)) &
				0x00FF0000) >> 16))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need match with the time
		 * stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		/* sw sof counter */
		sof_count[module]++;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;
					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;
					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this irq
		 * type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SV_SOF_INT_ST | SV_SW_PASS1_DON_ST | SV_VS1_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;
}

irqreturn_t ISP_Irq_CAMSV_3(int  Irq, void *DeviceId)
{
	/* LOG_DBG("ISP_IRQ_CAM_SV:0x%x\n",Irq); */
	unsigned int module = ISP_IRQ_TYPE_INT_CAMSV_3_ST;
	unsigned int reg_module = ISP_CAMSV3_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	/* */
	union FBC_CTRL_2 fbc_ctrl2[2];
	/* */
	unsigned int time_stamp;
	/* */
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;

	/* Avoid to touch hw module when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*  */
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAMSV_REG_INT_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];
	/*  */
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module));
	time_stamp       = ISP_RD32(CAMSV_REG_TG_TIME_STAMP(reg_module));

	/* sof , done order chech . */
	if ((IrqStatus & SV_HW_PASS1_DON_ST) || (IrqStatus & SV_SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & SV_HW_PASS1_DON_ST) && (IrqStatus & SV_SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & SV_SW_PASS1_DON_ST) {
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */
		/* update pass1 done time stamp for eis user(need match with
		 * the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
#define TMP_STR "CAMSV3 P1_DON_%d(0x%08x_0x%08x) stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR,
				(sof_count[module]) ?
					(sof_count[module] - 1) :
					(sof_count[module]),
				(unsigned int)(fbc_ctrl1[0].Raw),
				(unsigned int)(fbc_ctrl2[0].Raw),
				time_stamp);
#undef TMP_STR
		}

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
			pstRTBuf[module]->ring_buf[_camsv_imgo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SV_SOF_INT_ST) {
		time = ktime_get();     /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

#if 0 /* always return CAM_FST_DROP_FRAME for CAMSV0~CAMSV5 */
		/* chk this frame have EOF or not, dynimic dma port chk */
		/* ISP_Irq_CAMSV_3 */
		FrameStatus[module] = Irq_CAM_FrameStatus(reg_module);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMSV3 Lost p1 done_%d (0x%08x): ",
				       sof_count[module], cur_v_cnt);
		}
#endif

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
#if 0
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
			else
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
#endif
			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
#define TMP_STR1 "CAMSV3 P1_SOF_%d_%d(0x%08x_0x%08x,0x%08x),"
#define TMP_STR2 "int_us:0x%08x, stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module))),
				ISP_RD32(CAMSV_REG_IMGO_BASE_ADDR(reg_module)),
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				time_stamp);
#undef TMP_STR1
#undef TMP_STR2
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				((ISP_RD32(CAMSV_REG_TG_INTER_ST(reg_module)) &
				0x00FF0000) >> 16))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need to match with the
		 * time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		/* sw sof counter */
		sof_count[module]++;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;
					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;
					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this irq
		 * type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SV_SOF_INT_ST | SV_SW_PASS1_DON_ST | SV_VS1_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;
}

irqreturn_t ISP_Irq_CAMSV_4(int  Irq, void *DeviceId)
{
	/* LOG_DBG("ISP_IRQ_CAM_SV:0x%x\n",Irq); */
	unsigned int module = ISP_IRQ_TYPE_INT_CAMSV_4_ST;
	unsigned int reg_module = ISP_CAMSV4_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	/* */
	union FBC_CTRL_2 fbc_ctrl2[2];
	/* */
	unsigned int time_stamp;
	/* */
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;

	/* Avoid to touch hwmodule when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*  */
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAMSV_REG_INT_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];
	/*  */
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module));
	time_stamp       = ISP_RD32(CAMSV_REG_TG_TIME_STAMP(reg_module));

	/* sof , done order chech . */
	if ((IrqStatus & SV_HW_PASS1_DON_ST) || (IrqStatus & SV_SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & SV_HW_PASS1_DON_ST) && (IrqStatus & SV_SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & SV_SW_PASS1_DON_ST) {
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */
		/* update pass1 done time stamp for eis user(need to match with
		 * the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
#define TMP_STR "CAMSV4 P1_DON_%d(0x%08x_0x%08x) stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR,
				(sof_count[module]) ?
					(sof_count[module] - 1) :
					(sof_count[module]),
				(unsigned int)(fbc_ctrl1[0].Raw),
				(unsigned int)(fbc_ctrl2[0].Raw),
				time_stamp);
#undef TMP_STR
		}

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
			pstRTBuf[module]->ring_buf[_camsv_imgo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SV_SOF_INT_ST) {
		time = ktime_get();     /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

#if 0 /* always return CAM_FST_DROP_FRAME for CAMSV0~CAMSV5 */
		/* chk this frame have EOF or not, dynimic dma port chk */
		/* ISP_Irq_CAMSV_4 */
		FrameStatus[module] = Irq_CAM_FrameStatus(reg_module);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMSV4 Lost p1 done_%d (0x%08x): ",
				       sof_count[module], cur_v_cnt);
		}
#endif

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
#if 0
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
			else
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
#endif
			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
#define TMP_STR1 "CAMSV4 P1_SOF_%d_%d(0x%08x_0x%08x,0x%08x),"
#define TMP_STR2 "int_us:0x%08x, stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module))),
				ISP_RD32(CAMSV_REG_IMGO_BASE_ADDR(reg_module)),
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				time_stamp);
#undef TMP_STR1
#undef TMP_STR2
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				((ISP_RD32(CAMSV_REG_TG_INTER_ST(reg_module)) &
				0x00FF0000) >> 16))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need match with the time
		 * stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		/* sw sof counter */
		sof_count[module]++;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;

					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;

					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this
		 * irq type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SV_SOF_INT_ST | SV_SW_PASS1_DON_ST | SV_VS1_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;

}

irqreturn_t ISP_Irq_CAMSV_5(int  Irq, void *DeviceId)
{
	/* LOG_DBG("ISP_IRQ_CAM_SV:0x%x\n",Irq); */
	unsigned int module = ISP_IRQ_TYPE_INT_CAMSV_5_ST;
	unsigned int reg_module = ISP_CAMSV5_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	/* */
	union FBC_CTRL_2 fbc_ctrl2[2];
	/* */
	unsigned int time_stamp;
	/* */
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;

	/* Avoid to touch hw module when clock is disabled. DEVAPC will moniter
	 * this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*  */
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);     /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAMSV_REG_INT_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];
	/*  */
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module));
	time_stamp       = ISP_RD32(CAMSV_REG_TG_TIME_STAMP(reg_module));

	/* sof , done order chech . */
	if ((IrqStatus & SV_HW_PASS1_DON_ST) || (IrqStatus & SV_SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & SV_HW_PASS1_DON_ST) && (IrqStatus & SV_SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & SV_SW_PASS1_DON_ST) {
		sec = cpu_clock(0);     /* ns */
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */
		/* update pass1 done time stamp for eis user(need match with the
		 * time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
#define TMP_STR "CAMSV5 P1_DON_%d(0x%08x_0x%08x) stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR,
				(sof_count[module]) ?
					(sof_count[module] - 1) :
					(sof_count[module]),
				(unsigned int)(fbc_ctrl1[0].Raw),
				(unsigned int)(fbc_ctrl2[0].Raw),
				time_stamp);
#undef TMP_STR
		}

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
			pstRTBuf[module]->ring_buf[_camsv_imgo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SV_SOF_INT_ST) {
		time = ktime_get();     /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

#if 0 /* always return CAM_FST_DROP_FRAME for CAMSV0~CAMSV5 */
		/* chk this frame have EOF or not, dynimic dma port chk */
		/* ISP_Irq_CAMSV_5 */
		FrameStatus[module] = Irq_CAM_FrameStatus(reg_module);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMSV5 Lost p1 done_%d (0x%08x): ",
				       sof_count[module], cur_v_cnt);
		}
#endif

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
#if 0
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_camsv_imgo_].active)
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
			else
				magic_num =
				ISP_RD32(CAMSV_REG_IMGO_FH_SPARE_3(reg_module));
#endif
			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
#define TMP_STR1 "CAMSV5 P1_SOF_%d_%d(0x%08x_0x%08x,0x%08x),"
#define TMP_STR2 "int_us:0x%08x, stamp[0x%08x]\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				(ISP_RD32(CAMSV_REG_FBC_IMGO_CTL2(reg_module))),
				ISP_RD32(CAMSV_REG_IMGO_BASE_ADDR(reg_module)),
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				time_stamp);
#undef TMP_STR1
#undef TMP_STR2
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				((ISP_RD32(CAMSV_REG_TG_INTER_ST(reg_module)) &
				0x00FF0000) >> 16))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need to match with the
		 * time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		/* sw sof counter */
		sof_count[module]++;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;

					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;

					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this
		 * irq type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SV_SOF_INT_ST | SV_SW_PASS1_DON_ST | SV_VS1_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;

}

irqreturn_t ISP_Irq_CAM_A(int Irq, void *DeviceId)
{
	unsigned int module = ISP_IRQ_TYPE_INT_CAM_A_ST;
	unsigned int reg_module = ISP_CAM_A_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	unsigned int DmaStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	union FBC_CTRL_2 fbc_ctrl2[2];
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;
	unsigned int IrqEnableOrig, IrqEnableNew;

	/* Avoid touch hwmodule when clock is disable.
	 * DEVAPC will moniter this kind of err
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*	*/
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);  /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAM_REG_CTL_RAW_INT_STATUS(reg_module));
	DmaStatus = ISP_RD32(CAM_REG_CTL_RAW_INT2_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];

	/* Check ERR/WRN ISR times, if it occurs too frequently, mark it for
	 * avoding keep enter ISR. It will happen KE.
	 */
	for (i = 0; i < ISP_ISR_MAX_NUM; i++) {
		/* Only check irq that un marked yet */
		if (!((IspInfo.IrqCntInfo.m_err_int_mark[module] & (1 << i)) ||
			(IspInfo.IrqCntInfo.m_warn_int_mark[module] &
			(1 << i)))) {

			if (ErrStatus & (1 << i))
				IspInfo.IrqCntInfo.m_err_int_cnt[module][i]++;

			if (WarnStatus & (1 << i))
				IspInfo.IrqCntInfo.m_warn_int_cnt[module][i]++;


			if (usec - IspInfo.IrqCntInfo.m_int_usec[module] <
				INT_ERR_WARN_TIMER_THREAS) {
				if (IspInfo.IrqCntInfo.m_err_int_cnt[module][i]
					>= INT_ERR_WARN_MAX_TIME)
					IspInfo.IrqCntInfo.m_err_int_mark[
						module] |= (1 << i);

				if (IspInfo.IrqCntInfo.m_warn_int_cnt[module][i]
					>= INT_ERR_WARN_MAX_TIME)
					IspInfo.IrqCntInfo.m_warn_int_mark[
						module] |= (1 << i);

			} else {
				IspInfo.IrqCntInfo.m_int_usec[module] = usec;
				IspInfo.IrqCntInfo.m_err_int_cnt[module][i] = 0;
				IspInfo.IrqCntInfo.m_warn_int_cnt[module][i] =
					0;
			}
		}

	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqEnableOrig = ISP_RD32(CAM_REG_CTL_RAW_INT_EN(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	IrqEnableNew =
		IrqEnableOrig & ~(IspInfo.IrqCntInfo.m_err_int_mark[module] |
		IspInfo.IrqCntInfo.m_warn_int_mark[module]);
	ISP_WR32(CAM_REG_CTL_RAW_INT_EN(reg_module), IrqEnableNew);

	/*	*/
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAM_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl1[1].Raw = ISP_RD32(CAM_REG_FBC_RRZO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAM_REG_FBC_IMGO_CTL2(reg_module));
	fbc_ctrl2[1].Raw = ISP_RD32(CAM_REG_FBC_RRZO_CTL2(reg_module));

	#if defined(ISP_MET_READY)
	/*MET:ISP EOF*/
	if (IrqStatus & HW_PASS1_DON_ST) {
		if (met_mmsys_event_isp_pass1_end)
			met_mmsys_event_isp_pass1_end(0);
		MET_Events_Trace(0, reg_module);
	}

	if (IrqStatus & SOF_INT_ST) {
		/*met mmsys profile*/
		if (met_mmsys_event_isp_pass1_begin)
			met_mmsys_event_isp_pass1_begin(0);
		MET_Events_Trace(1, reg_module);
	}
	#endif

	/* sof , done order chech . */
	if ((IrqStatus & HW_PASS1_DON_ST) || (IrqStatus & SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & HW_PASS1_DON_ST) && (IrqStatus & SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n", cur_v_cnt,
				sof_count[module]);
	}

	if ((IrqStatus & HW_PASS1_DON_ST) &&
		(IspInfo.DebugMask & ISP_DBG_HW_DON)) {
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
			"CAMA P1_HW_DON_%d\n",
			(sof_count[module]) ?
			(sof_count[module] - 1) : (sof_count[module]));
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & VS_INT_ST) {
		Vsync_cnt[0]++;
		/*LOG_INF("CAMA N3D:0x%x\n", Vsync_cnt[0]);*/
	}
	if (IrqStatus & SW_PASS1_DON_ST) {
		time = ktime_get(); /* ns */
		sec = time.tv64;
		do_div(sec, 1000);	  /* usec */
		usec = do_div(sec, 1000000);	/* sec and usec */
		/* update pass1 done time stamp for eis user(need to match
		 * with the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			/*SW p1_don is not reliable*/
			if (FrameStatus[module] != CAM_FST_DROP_FRAME) {
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"CAMA P1_DON_%d(0x%x_0x%x,0x%x_0x%x)\n",
					(sof_count[module]) ?
						(sof_count[module] - 1) :
						(sof_count[module]),
					(unsigned int)(fbc_ctrl1[0].Raw),
					(unsigned int)(fbc_ctrl2[0].Raw),
					(unsigned int)(fbc_ctrl1[1].Raw),
					(unsigned int)(fbc_ctrl2[1].Raw));
			}
		}

		#if (TSTMP_SUBSAMPLE_INTPL == 1)
		if (g1stSwP1Done[module] == MTRUE) {
			unsigned long long cur_timestp =
				(unsigned long long)sec * 1000000 + usec;
			unsigned int frmPeriod =
				((ISP_RD32(CAM_REG_TG_SUB_PERIOD(reg_module)) >>
				8) & 0x1F) + 1;

			if (frmPeriod > 1) {
				ISP_PatchTimestamp(module, _imgo_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _rrzo_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _ufeo_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _eiso_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _lcso_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
			}

			g1stSwP1Done[module] = MFALSE;
		}
		#endif

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_imgo_].active)
			pstRTBuf[module]->ring_buf[_imgo_].img_cnt =
				sof_count[module];

		if (pstRTBuf[module]->ring_buf[_rrzo_].active)
			pstRTBuf[module]->ring_buf[_rrzo_].img_cnt =
				sof_count[module];
		if (pstRTBuf[module]->ring_buf[_ufeo_].active)
			pstRTBuf[module]->ring_buf[_ufeo_].img_cnt =
				sof_count[module];
	}

	if (IrqStatus & SOF_INT_ST) {
		unsigned int frmPeriod =
			((ISP_RD32(CAM_REG_TG_SUB_PERIOD(reg_module)) >> 8) &
			0x1F) + 1;
		unsigned int irqDelay = 0;

		time = ktime_get(); /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

		if (frmPeriod == 0) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"ERROR: Wrong sub-sample period: 0");
			goto LB_CAMA_SOF_IGNORE;
		}

		/* chk this frame have EOF or not, dynimic dma port chk */
		FrameStatus[module] =
			Irq_CAM_FrameStatus(reg_module, module, irqDelay);

		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMA Lost p1 done_%d (0x%x): ",
				sof_count[module], cur_v_cnt);
		}

		/* During SOF, re-enable that err/warn irq had been marked and
		 * reset IrqCntInfo
		 */
		IrqEnableNew = ISP_RD32(CAM_REG_CTL_RAW_INT_EN(reg_module));
		IrqEnableNew |= (IspInfo.IrqCntInfo.m_err_int_mark[module] |
			IspInfo.IrqCntInfo.m_warn_int_mark[module]);
		ISP_WR32(CAM_REG_CTL_RAW_INT_EN(reg_module), IrqEnableNew);

		IspInfo.IrqCntInfo.m_err_int_mark[module] = 0;
		IspInfo.IrqCntInfo.m_warn_int_mark[module] = 0;
		IspInfo.IrqCntInfo.m_int_usec[module] = 0;

		for (i = 0; i < ISP_ISR_MAX_NUM; i++) {
			IspInfo.IrqCntInfo.m_err_int_cnt[module][i] = 0;
			IspInfo.IrqCntInfo.m_warn_int_cnt[module][i] = 0;
		}

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_imgo_].active)
				magic_num =
				ISP_RD32(CAM_REG_IMGO_FH_SPARE_2(reg_module));
			else
				magic_num =
				ISP_RD32(CAM_REG_RRZO_FH_SPARE_2(reg_module));

			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}

			#if (TIMESTAMP_QUEUE_EN == 1)
			{
			unsigned long long cur_timestp =
				(unsigned long long)sec*1000000 + usec;
			unsigned int subFrm = 0;
			enum CAM_FrameST FrmStat_aao, FrmStat_afo;
			enum CAM_FrameST FrmStat_flko, FrmStat_pdo;
			enum CAM_FrameST FrmStat_pso;

			ISP_GetDmaPortsStatus(reg_module,
				IspInfo.TstpQInfo[module].DmaEnStatus);

			/* Prevent WCNT increase after
			 * ISP_CompensateMissingSofTime around P1_DON and
			 * FBC_CNT decrease to 0, following drop frame is
			 * checked becomes true, then SOF timestamp will missing
			 * for current frame.
			 */
			if (IspInfo.TstpQInfo[module].DmaEnStatus[_aao_])
				FrmStat_aao = Irq_CAM_SttFrameStatus(reg_module,
					module, _aao_, irqDelay);
			else
				FrmStat_aao = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_afo_])
				FrmStat_afo = Irq_CAM_SttFrameStatus(reg_module,
					module, _afo_, irqDelay);
			else
				FrmStat_afo = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_flko_])
				FrmStat_flko = Irq_CAM_SttFrameStatus(
					reg_module, module, _flko_, irqDelay);
			else
				FrmStat_flko = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pdo_])
				FrmStat_pdo = Irq_CAM_SttFrameStatus(
					reg_module, module, _pdo_, irqDelay);
			else
				FrmStat_pdo = CAM_FST_DROP_FRAME;
			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pso_])
				FrmStat_pso = Irq_CAM_SttFrameStatus(reg_module,
					module, _pso_, irqDelay);
			else
				FrmStat_pso = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_imgo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_imgo_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_rrzo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_rrzo_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_ufeo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_ufeo_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_eiso_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_eiso_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_lcso_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_lcso_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_aao_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_aao_, sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_afo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_afo_, sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_flko_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_flko_, sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pdo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_pdo_, sec, usec, 1);
			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pso_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_pso_, sec, usec, 1);

			if (FrameStatus[module] != CAM_FST_DROP_FRAME) {
				for (subFrm = 0; subFrm < frmPeriod; subFrm++) {
					/* Current frame is NOT DROP FRAME */
					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_imgo_])
						ISP_PushBufTimestamp(module,
							_imgo_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_rrzo_])
						ISP_PushBufTimestamp(module,
							_rrzo_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_ufeo_])
						ISP_PushBufTimestamp(module,
							_ufeo_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_eiso_])
						ISP_PushBufTimestamp(module,
							_eiso_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_lcso_])
						ISP_PushBufTimestamp(module,
							_lcso_, sec, usec,
							frmPeriod);
				}

				/* for slow motion sub-sample */
				/* must after current ISP_PushBufTimestamp() */
				#if (TSTMP_SUBSAMPLE_INTPL == 1)
				if ((frmPeriod > 1) &&
					(g1stSwP1Done[module] == MFALSE)) {
					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_imgo_])
						ISP_PatchTimestamp(module,
							_imgo_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_rrzo_])
						ISP_PatchTimestamp(module,
							_rrzo_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_ufeo_])
						ISP_PatchTimestamp(module,
							_ufeo_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_eiso_])
						ISP_PatchTimestamp(module,
							_eiso_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_lcso_])
						ISP_PatchTimestamp(module,
							_lcso_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);
				}
				#endif
			}

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_aao_])
				if (FrmStat_aao != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _aao_,
						sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_afo_])
				if (FrmStat_afo != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _afo_,
						sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_flko_])
				if (FrmStat_flko != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _flko_,
						sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pdo_])
				if (FrmStat_pdo != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _pdo_,
						sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pso_])
				if (FrmStat_pso != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _pso_,
						sec, usec, 1);
			#if (TSTMP_SUBSAMPLE_INTPL == 1)
			gPrevSofTimestp[module] = cur_timestp;
			#endif

			}
			#endif /* (TIMESTAMP_QUEUE_EN == 1) */
#define TMP_STR1 "CAMA P1_SOF_%d_%d(0x%x_0x%x,0x%x_0x%x,0x%x,0x%x,0x%x),"
#define TMP_STR2 "int_us:%d,cq:0x%x\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_IMGO_CTL2(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_RRZO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_RRZO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_IMGO_BASE_ADDR(reg_module)),
				ISP_RD32(CAM_REG_RRZO_BASE_ADDR(reg_module)),
				magic_num,
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				ISP_RD32(CAM_REG_CQ_THR0_BASEADDR(reg_module)));
#undef TMP_STR1
#undef TMP_STR2
#ifdef ENABLE_STT_IRQ_LOG /*STT addr*/
#define TMP_STR1 "CAMA_aa(0x%x_0x%x_0x%x)af(0x%x_0x%x_0x%x),"
#define TMP_STR2 "pd(0x%x_0x%x_0x%x),ps(0x%x_0x%x_0x%x)\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				ISP_RD32(CAM_REG_AAO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AAO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AAO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_AFO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AFO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AFO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_PDO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PDO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PDO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_PSO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PSO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PSO_CTL2(reg_module))));
#undef TMP_STR1
#undef TMP_STR2
#endif
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				ISP_RD32_TG_CAM_FRM_CNT(module, reg_module))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need match with the time
		 * stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		#if 0
		/* sw sof counter */
		if (irqDelay == 1)
			sof_count[module] = sofCntGrpHw;
		#endif

		sof_count[module] += frmPeriod;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}
LB_CAMA_SOF_IGNORE:

#ifdef ENABLE_STT_IRQ_LOG
	if (DmaStatus & (AAO_DONE_ST|AFO_DONE_ST|PDO_DONE_ST|PSO_DONE_ST)) {
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
			"CAMA_STT_Done_%d_0x%x\n",
			(sof_count[module]) ? (sof_count[module] - 1) :
				(sof_count[module]), DmaStatus);
	}
#endif

	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;
		IspInfo.IrqInfo.Status[module][DMA_INT][i] |= DmaStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;
					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;
					IspInfo.IrqInfo.PassedBySigCnt[
						module][cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask
		 * in this irq type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SOF_INT_ST | SW_PASS1_DON_ST | VS_INT_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;
}

irqreturn_t ISP_Irq_CAM_B(int  Irq, void *DeviceId)
{
	unsigned int module = ISP_IRQ_TYPE_INT_CAM_B_ST;
	unsigned int reg_module = ISP_CAM_B_IDX;
	unsigned int i;
	unsigned int IrqStatus, ErrStatus, WarnStatus;
	unsigned int DmaStatus;
	union FBC_CTRL_1 fbc_ctrl1[2];
	union FBC_CTRL_2 fbc_ctrl2[2];
	unsigned int cur_v_cnt = 0;
	struct timeval time_frmb;
	unsigned long long  sec = 0;
	unsigned long       usec = 0;
	ktime_t             time;
	unsigned int IrqEnableOrig, IrqEnableNew;

	/* Avoid touch hwmodule when clock is disable.
	 * DEVAPC will moniter this kind of err.
	 */
	if (G_u4EnableClockCount == 0)
		return IRQ_HANDLED;

	/*	*/
	/* do_gettimeofday(&time_frmb); */
	sec = cpu_clock(0);  /* ns */
	do_div(sec, 1000);    /* usec */
	usec = do_div(sec, 1000000);    /* sec and usec */
	time_frmb.tv_usec = usec;
	time_frmb.tv_sec = sec;

	#if (ISP_BOTTOMHALF_WORKQ == 1)
	gSvLog[module]._lastIrqTime.sec = sec;
	gSvLog[module]._lastIrqTime.usec = usec;
	#endif

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqStatus = ISP_RD32(CAM_REG_CTL_RAW_INT_STATUS(reg_module));
	DmaStatus = ISP_RD32(CAM_REG_CTL_RAW_INT2_STATUS(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	ErrStatus = IrqStatus & IspInfo.IrqInfo.ErrMask[module][SIGNAL_INT];
	WarnStatus = IrqStatus & IspInfo.IrqInfo.WarnMask[module][SIGNAL_INT];
	IrqStatus = IrqStatus & IspInfo.IrqInfo.Mask[module][SIGNAL_INT];

	/* Check ERR/WRN ISR times, if it occur too frequently, mark it for
	 * avoding keep enter ISR. It will happen KE
	 */
	for (i = 0; i < ISP_ISR_MAX_NUM; i++) {
		/* Only check irq that un marked yet */
		if (!((IspInfo.IrqCntInfo.m_err_int_mark[module] & (1 << i)) ||
			(IspInfo.IrqCntInfo.m_warn_int_mark[module] &
			(1 << i)))) {

			if (ErrStatus & (1 << i))
				IspInfo.IrqCntInfo.m_err_int_cnt[module][i]++;

			if (WarnStatus & (1 << i))
				IspInfo.IrqCntInfo.m_warn_int_cnt[module][i]++;


			if (usec - IspInfo.IrqCntInfo.m_int_usec[module] <
				INT_ERR_WARN_TIMER_THREAS) {
				if (IspInfo.IrqCntInfo.m_err_int_cnt[module][
					i] >= INT_ERR_WARN_MAX_TIME)
					IspInfo.IrqCntInfo.m_err_int_mark[
						module] |= (1 << i);

				if (IspInfo.IrqCntInfo.m_warn_int_cnt[module][
					i] >= INT_ERR_WARN_MAX_TIME)
					IspInfo.IrqCntInfo.m_warn_int_mark[
						module] |= (1 << i);

			} else {
				IspInfo.IrqCntInfo.m_int_usec[module] = usec;
				IspInfo.IrqCntInfo.m_err_int_cnt[module][i] = 0;
				IspInfo.IrqCntInfo.m_warn_int_cnt[
					module][i] = 0;
			}
		}

	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	IrqEnableOrig = ISP_RD32(CAM_REG_CTL_RAW_INT_EN(reg_module));
	spin_unlock(&(IspInfo.SpinLockIrq[module]));

	IrqEnableNew =
		IrqEnableOrig & ~(IspInfo.IrqCntInfo.m_err_int_mark[module] |
		IspInfo.IrqCntInfo.m_warn_int_mark[module]);
	ISP_WR32(CAM_REG_CTL_RAW_INT_EN(reg_module), IrqEnableNew);

	/*	*/
	IRQ_INT_ERR_CHECK_CAM(WarnStatus, ErrStatus, module);

	fbc_ctrl1[0].Raw = ISP_RD32(CAM_REG_FBC_IMGO_CTL1(reg_module));
	fbc_ctrl1[1].Raw = ISP_RD32(CAM_REG_FBC_RRZO_CTL1(reg_module));
	fbc_ctrl2[0].Raw = ISP_RD32(CAM_REG_FBC_IMGO_CTL2(reg_module));
	fbc_ctrl2[1].Raw = ISP_RD32(CAM_REG_FBC_RRZO_CTL2(reg_module));


	#if defined(ISP_MET_READY)
	/*CLS:Test*/
	if (IrqStatus & HW_PASS1_DON_ST) {
		if (met_mmsys_event_isp_pass1_end)
			met_mmsys_event_isp_pass1_end(1);
		MET_Events_Trace(0, reg_module);
	}

	if (IrqStatus & SOF_INT_ST) {
		/*met mmsys profile*/
		if (met_mmsys_event_isp_pass1_begin)
			met_mmsys_event_isp_pass1_begin(1);
		MET_Events_Trace(1, reg_module);
	}
	#endif

	/* sof , done order chech . */
	if ((IrqStatus & HW_PASS1_DON_ST) || (IrqStatus & SOF_INT_ST))
		cur_v_cnt = ISP_RD32_TG_CAM_FRM_CNT(module, reg_module);

	if ((IrqStatus & HW_PASS1_DON_ST) && (IrqStatus & SOF_INT_ST)) {
		if (cur_v_cnt != sof_count[module])
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"isp sof_don block, %d_%d\n",
				cur_v_cnt, sof_count[module]);
	}

	if ((IrqStatus & HW_PASS1_DON_ST) &&
		(IspInfo.DebugMask & ISP_DBG_HW_DON)) {
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
			"CAMB P1_HW_DON_%d\n", (sof_count[module]) ?
			(sof_count[module] - 1) : (sof_count[module]));
	}

	spin_lock(&(IspInfo.SpinLockIrq[module]));
	if (IrqStatus & VS_INT_ST) {
		Vsync_cnt[1]++;
		/* LOG_INF("CAMB N3D:0x%x\n",Vsync_cnt[1]); */
	}
	if (IrqStatus & SW_PASS1_DON_ST) {
		time = ktime_get(); /* ns */
		sec = time.tv64;
		do_div(sec, 1000);	  /* usec */
		usec = do_div(sec, 1000000);	/* sec and usec */
		/* update pass1 done time stamp for eis user(need match with
		 * the time stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][10] =
			(unsigned int)(usec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][10] =
			(unsigned int)(sec);

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			/*SW p1_don is not reliable*/
			if (FrameStatus[module] != CAM_FST_DROP_FRAME) {
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
					"CAMB P1_DON_%d(0x%x_0x%x,0x%x_0x%x)\n",
					(sof_count[module]) ?
						(sof_count[module] - 1) :
						(sof_count[module]),
					(unsigned int)(fbc_ctrl1[0].Raw),
					(unsigned int)(fbc_ctrl2[0].Raw),
					(unsigned int)(fbc_ctrl1[1].Raw),
					(unsigned int)(fbc_ctrl2[1].Raw));
			}
		}

		#if (TSTMP_SUBSAMPLE_INTPL == 1)
		if (g1stSwP1Done[module] == MTRUE) {
			unsigned long long cur_timestp =
				(unsigned long long)sec * 1000000 + usec;
			unsigned int frmPeriod =
				((ISP_RD32(CAM_REG_TG_SUB_PERIOD(reg_module)) >>
				8) & 0x1F) + 1;

			if (frmPeriod > 1) {
				ISP_PatchTimestamp(module, _imgo_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _rrzo_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _ufeo_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _eiso_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
				ISP_PatchTimestamp(module, _lcso_, frmPeriod,
					cur_timestp, gPrevSofTimestp[module]);
			}

			g1stSwP1Done[module] = MFALSE;
		}
		#endif

		/* for dbg log only */
		if (pstRTBuf[module]->ring_buf[_imgo_].active)
			pstRTBuf[module]->ring_buf[_imgo_].img_cnt =
				sof_count[module];

		if (pstRTBuf[module]->ring_buf[_rrzo_].active)
			pstRTBuf[module]->ring_buf[_rrzo_].img_cnt =
				sof_count[module];

	}

	if (IrqStatus & SOF_INT_ST) {
		unsigned int frmPeriod =
			((ISP_RD32(CAM_REG_TG_SUB_PERIOD(reg_module)) >> 8) &
			0x1F) + 1;

		unsigned int irqDelay = 0;

		time = ktime_get(); /* ns */
		sec = time.tv64;
		do_div(sec, 1000);    /* usec */
		usec = do_div(sec, 1000000);    /* sec and usec */

		if (frmPeriod == 0) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_ERR,
				"ERROR: Wrong sub-sample period: 0");
			goto LB_CAMB_SOF_IGNORE;
		}

		/* chk this frame have EOF or not, dynimic dma port chk */
		FrameStatus[module] =
			Irq_CAM_FrameStatus(reg_module, module, irqDelay);
		if (FrameStatus[module] == CAM_FST_DROP_FRAME) {
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"CAMB Lost p1 done_%d (0x%x): ",
				sof_count[module], cur_v_cnt);
		}

		/* During SOF, re-enable that err/warn irq had been marked and
		 * reset IrqCntInfo
		 */
		IrqEnableNew = ISP_RD32(CAM_REG_CTL_RAW_INT_EN(reg_module));
		IrqEnableNew |= (IspInfo.IrqCntInfo.m_err_int_mark[module] |
			IspInfo.IrqCntInfo.m_warn_int_mark[module]);
		ISP_WR32(CAM_REG_CTL_RAW_INT_EN(reg_module), IrqEnableNew);

		IspInfo.IrqCntInfo.m_err_int_mark[module] = 0;
		IspInfo.IrqCntInfo.m_warn_int_mark[module] = 0;
		IspInfo.IrqCntInfo.m_int_usec[module] = 0;

		for (i = 0; i < ISP_ISR_MAX_NUM; i++) {
			IspInfo.IrqCntInfo.m_err_int_cnt[module][i] = 0;
			IspInfo.IrqCntInfo.m_warn_int_cnt[module][i] = 0;
		}

		if (IspInfo.DebugMask & ISP_DBG_INT) {
			static unsigned int m_sec = 0, m_usec;
			unsigned int magic_num;

			if (pstRTBuf[module]->ring_buf[_imgo_].active)
				magic_num =
				ISP_RD32(CAM_REG_IMGO_FH_SPARE_2(reg_module));
			else
				magic_num =
				ISP_RD32(CAM_REG_RRZO_FH_SPARE_2(reg_module));

			if (g1stSof[module]) {
				m_sec = sec;
				m_usec = usec;
				gSTime[module].sec = sec;
				gSTime[module].usec = usec;
			}
			#if (TIMESTAMP_QUEUE_EN == 1)
			{
			unsigned long long cur_timestp =
				(unsigned long long)sec * 1000000 + usec;
			unsigned int subFrm = 0;
			enum CAM_FrameST FrmStat_aao, FrmStat_afo;
			enum CAM_FrameST FrmStat_flko, FrmStat_pdo;
			enum CAM_FrameST FrmStat_pso;

			ISP_GetDmaPortsStatus(reg_module,
				IspInfo.TstpQInfo[module].DmaEnStatus);

			/* Prevent WCNT increase after
			 * ISP_CompensateMissingSofTime around P1_DON and
			 * FBC_CNT decrease to 0, following drop frame is
			 * checked becomes true, then SOF timestamp will missing
			 * for current frame.
			 */
			if (IspInfo.TstpQInfo[module].DmaEnStatus[_aao_])
				FrmStat_aao = Irq_CAM_SttFrameStatus(
					reg_module, module, _aao_, irqDelay);
			else
				FrmStat_aao = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_afo_])
				FrmStat_afo = Irq_CAM_SttFrameStatus(
					reg_module, module, _afo_, irqDelay);
			else
				FrmStat_afo = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_flko_])
				FrmStat_flko = Irq_CAM_SttFrameStatus(
					reg_module, module, _flko_, irqDelay);
			else
				FrmStat_flko = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pdo_])
				FrmStat_pdo = Irq_CAM_SttFrameStatus(
					reg_module, module, _pdo_, irqDelay);
			else
				FrmStat_pdo = CAM_FST_DROP_FRAME;
			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pso_])
				FrmStat_pso = Irq_CAM_SttFrameStatus(
					reg_module, module, _pso_, irqDelay);
			else
				FrmStat_pso = CAM_FST_DROP_FRAME;

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_imgo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_imgo_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_rrzo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_rrzo_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_ufeo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_ufeo_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_eiso_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_eiso_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_lcso_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_lcso_, sec, usec, frmPeriod);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_aao_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_aao_, sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_afo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_afo_, sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_flko_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_flko_, sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pdo_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_pdo_, sec, usec, 1);
			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pso_])
				ISP_CompensateMissingSofTime(reg_module, module,
					_pso_, sec, usec, 1);

			if (FrameStatus[module] != CAM_FST_DROP_FRAME) {
				for (subFrm = 0; subFrm < frmPeriod; subFrm++) {
					/* Current frame is NOT DROP FRAME */
					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_imgo_])
						ISP_PushBufTimestamp(module,
							_imgo_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_rrzo_])
						ISP_PushBufTimestamp(module,
							_rrzo_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_ufeo_])
						ISP_PushBufTimestamp(module,
							_ufeo_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_eiso_])
						ISP_PushBufTimestamp(module,
							_eiso_, sec, usec,
							frmPeriod);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_lcso_])
						ISP_PushBufTimestamp(module,
							_lcso_, sec, usec,
							frmPeriod);
				}

				/* for slow motion sub-sample */
				/* must after current ISP_PushBufTimestamp() */
				#if (TSTMP_SUBSAMPLE_INTPL == 1)
				if ((frmPeriod > 1) &&
					(g1stSwP1Done[module] == MFALSE)) {
					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_imgo_])
						ISP_PatchTimestamp(module,
							_imgo_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_rrzo_])
						ISP_PatchTimestamp(module,
							_rrzo_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_ufeo_])
						ISP_PatchTimestamp(module,
							_ufeo_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_eiso_])
						ISP_PatchTimestamp(module,
							_eiso_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);

					if (IspInfo.TstpQInfo[module].
						DmaEnStatus[_lcso_])
						ISP_PatchTimestamp(module,
							_lcso_, frmPeriod,
							cur_timestp,
							gPrevSofTimestp[
							module]);
				}
				#endif
			}

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_aao_])
				if (FrmStat_aao != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _aao_, sec,
						usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_afo_])
				if (FrmStat_afo != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _afo_, sec,
						usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_flko_])
				if (FrmStat_flko != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _flko_,
						sec, usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pdo_])
				if (FrmStat_pdo != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _pdo_, sec,
						usec, 1);

			if (IspInfo.TstpQInfo[module].DmaEnStatus[_pso_])
				if (FrmStat_pso != CAM_FST_DROP_FRAME)
					ISP_PushBufTimestamp(module, _pso_, sec,
						usec, 1);
			#if (TSTMP_SUBSAMPLE_INTPL == 1)
			gPrevSofTimestp[module] = cur_timestp;
			#endif

			}
			#endif /* (TIMESTAMP_QUEUE_EN == 1) */
#define TMP_STR1 "CAMB P1_SOF_%d_%d(0x%x_0x%x,0x%x_0x%x,0x%x,0x%x,0x%x),"
#define TMP_STR2 "int_us:%d,cq:0x%x\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				sof_count[module], cur_v_cnt,
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_IMGO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_IMGO_CTL2(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_RRZO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_RRZO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_IMGO_BASE_ADDR(reg_module)),
				ISP_RD32(CAM_REG_RRZO_BASE_ADDR(reg_module)),
				magic_num,
				(unsigned int)((sec * 1000000 + usec) -
					(1000000 * m_sec + m_usec)),
				ISP_RD32(CAM_REG_CQ_THR0_BASEADDR(reg_module)));
#undef TMP_STR1
#undef TMP_STR2

#ifdef ENABLE_STT_IRQ_LOG /*STT addr*/
#define TMP_STR1 "CAMB_aa(0x%x_0x%x_0x%x)af(0x%x_0x%x_0x%x),"
#define TMP_STR2 "pd(0x%x_0x%x_0x%x),ps(0x%x_0x%x_0x%x)\n"
			IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				TMP_STR1 TMP_STR2,
				ISP_RD32(CAM_REG_AAO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AAO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AAO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_AFO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AFO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_AFO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_PDO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PDO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PDO_CTL2(reg_module))),
				ISP_RD32(CAM_REG_PSO_BASE_ADDR(reg_module)),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PSO_CTL1(reg_module))),
				(unsigned int)
				  (ISP_RD32(CAM_REG_FBC_PSO_CTL2(reg_module))));
#undef TMP_STR1
#undef TMP_STR2
#endif
			/* keep current time */
			m_sec = sec;
			m_usec = usec;

			/* dbg information only */
			if (cur_v_cnt !=
				ISP_RD32_TG_CAM_FRM_CNT(module, reg_module))
				IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
				"SW ISR right on next hw p1_done\n");

		}

		/* update SOF time stamp for eis user(need match with the time
		 * stamp in image header)
		 */
		IspInfo.IrqInfo.LastestSigTime_usec[module][12] =
			(unsigned int)(sec);
		IspInfo.IrqInfo.LastestSigTime_sec[module][12] =
			(unsigned int)(usec);

		#if 0
		/* sw sof counter */
		if (irqDelay == 1)
			sof_count[module] = sofCntGrpHw;
		#endif

		sof_count[module] += frmPeriod;
		/* for match vsync cnt */
		if (sof_count[module] > 255)
			sof_count[module] -= 256;

		g1stSof[module] = MFALSE;
	}
LB_CAMB_SOF_IGNORE:

#ifdef ENABLE_STT_IRQ_LOG
	if (DmaStatus & (AAO_DONE_ST|AFO_DONE_ST|PDO_DONE_ST|PSO_DONE_ST)) {
		IRQ_LOG_KEEPER(module, m_CurrentPPB, _LOG_INF,
			"CAMB_STT_Done_%d_0x%x\n",
			(sof_count[module]) ? (sof_count[module] - 1) :
			(sof_count[module]), DmaStatus);
	}
#endif
	for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
		/* 1. update interrupt status to all users */
		IspInfo.IrqInfo.Status[module][SIGNAL_INT][i] |= IrqStatus;
		IspInfo.IrqInfo.Status[module][DMA_INT][i] |= DmaStatus;

		/* 2. update signal time and passed by signal count */
		if (IspInfo.IrqInfo.MarkedFlag[module][SIGNAL_INT][i] &
			IspInfo.IrqInfo.Mask[module][SIGNAL_INT]) {
			unsigned int cnt = 0, tmp = IrqStatus;

			while (tmp) {
				if (tmp & 0x1) {
					IspInfo.IrqInfo.LastestSigTime_usec[
						module][cnt] =
						(unsigned int)time_frmb.tv_usec;

					IspInfo.IrqInfo.LastestSigTime_sec[
						module][cnt] =
						(unsigned int) time_frmb.tv_sec;

					IspInfo.IrqInfo.PassedBySigCnt[module][
						cnt][i]++;
				}
				tmp = tmp >> 1;
				cnt++;
			}
		} else {
		/* no any interrupt is not marked and in read mask in this irq
		 * type.
		 */
		}
	}
	spin_unlock(&(IspInfo.SpinLockIrq[module]));
	/*  */
	wake_up_interruptible(&IspInfo.WaitQueueHead[module]);

	/* dump log, use tasklet */
	if (IrqStatus & (SOF_INT_ST | SW_PASS1_DON_ST | VS_INT_ST)) {
		#if (ISP_BOTTOMHALF_WORKQ == 1)
		schedule_work(&isp_workque[module].isp_bh_work);
		#else
		tasklet_schedule(isp_tasklet[module].pIsp_tkt);
		#endif
	}

	return IRQ_HANDLED;
}


/*******************************************************************************
 *
 ******************************************************************************/


static void ISP_TaskletFunc_CAM_A(unsigned long data)
{
#if 1
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAM_A_ST, m_CurrentPPB, _LOG_INF);
#else
	unsigned int reg, flags;

	spin_lock_irqsave(
		&(IspInfo.SpinLockIrq[ISP_IRQ_TYPE_INT_CAM_A_ST]), flags);
	reg = IspInfo.IrqInfo.Status[ISP_IRQ_TYPE_INT_CAM_A_ST][0];
	spin_unlock_irqrestore(
		&(IspInfo.SpinLockIrq[ISP_IRQ_TYPE_INT_CAM_A_ST]), flags);
	LOG_DBG("ISP_IRQ_CAM_A Status User0: 0x%x\n", reg);

#endif
}

static void ISP_TaskletFunc_CAM_B(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAM_B_ST, m_CurrentPPB, _LOG_INF);
}

static void ISP_TaskletFunc_SV_0(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAMSV_0_ST, m_CurrentPPB, _LOG_INF);
}

static void ISP_TaskletFunc_SV_1(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAMSV_1_ST, m_CurrentPPB, _LOG_INF);
}

static void ISP_TaskletFunc_SV_2(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAMSV_2_ST, m_CurrentPPB, _LOG_INF);
}

static void ISP_TaskletFunc_SV_3(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAMSV_3_ST, m_CurrentPPB, _LOG_INF);
}

static void ISP_TaskletFunc_SV_4(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAMSV_4_ST, m_CurrentPPB, _LOG_INF);
}

static void ISP_TaskletFunc_SV_5(unsigned long data)
{
	IRQ_LOG_PRINTER(ISP_IRQ_TYPE_INT_CAMSV_5_ST, m_CurrentPPB, _LOG_INF);
}

#if (ISP_BOTTOMHALF_WORKQ == 1)
static void ISP_BH_Workqueue(struct work_struct *pWork)
{
	struct IspWorkqueTable *pWorkTable =
		container_of(pWork, struct IspWorkqueTable, isp_bh_work);

	IRQ_LOG_PRINTER(pWorkTable->module, m_CurrentPPB, _LOG_ERR);
	IRQ_LOG_PRINTER(pWorkTable->module, m_CurrentPPB, _LOG_INF);
}
#endif

/*******************************************************************************
 *
 ******************************************************************************/
module_init(ISP_Init);
module_exit(ISP_Exit);
MODULE_DESCRIPTION("Camera ISP driver");
MODULE_AUTHOR("ME8");
MODULE_LICENSE("GPL");
