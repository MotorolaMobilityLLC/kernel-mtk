/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/


#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
/* #include <asm/io.h> */
/* #include <asm/tcm.h> */
#include <linux/proc_fs.h>	/* proc file use */
#include <linux/slab.h>		/* kmalloc/kfree in kernel 3.10 */
#include <linux/spinlock.h>
/* #include <linux/io.h> */
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <mt-plat/aee.h>
/* #include <mach/hardware.h> */
/* #include <mach/mt6589_pll.h> */
/* #include <mach/camera_isp.h> */
/* #include <mach/mt_reg_base.h> */
#include <mach/mt_clkmgr.h>	/* For clock mgr APIS. enable_clock()/disable_clock(). */
#include <mt-plat/sync_write.h>
/*#include <mach/mt_spm_idle.h>*/	/* For spm_enable_sodi()/spm_disable_sodi(). */

/*#include <smi_common.h>*/

#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
#else
#include <linux/wakelock.h>
#endif

#define LOG_CONSTRAINT_ADJ      (0)
#if (LOG_CONSTRAINT_ADJ == 1)
/* for kernel log reduction */
#include <linux/printk.h>
#endif

#include "inc/camera_isp_FrmB_D2.h"
#include "camera_isp_FrmB_D2.c"

#include <m4u.h>
#include <cmdq_core.h>

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif


#ifdef CONFIG_OF
#include <linux/of_platform.h>	/* for device tree */
#include <linux/of_irq.h>	/* for device tree */
#include <linux/of_address.h>	/* for device tree */
#endif


/*******************************************************************************
* common type define
********************************************************************************/
typedef unsigned char MUINT8;
typedef unsigned short MUINT16;
typedef unsigned int MUINT32;
typedef unsigned long long MUINT64;

typedef signed char MINT8;
typedef signed short MINT16;
typedef signed int MINT32;
typedef signed long long MINT64;

typedef float MFLOAT;

typedef void MVOID;
typedef bool MBOOL;

#ifndef MTRUE
#define MTRUE               1
#endif

#ifndef MFALSE
#define MFALSE              0
#endif

/*******************************************************************************
* LOG marco
********************************************************************************/
#define MyTag "[ISP_D2] "
#define LOG_DBG(format, args...)    pr_debug(MyTag format, ##args)
#define LOG_INF(format, args...)    pr_debug(MyTag format,  ##args)
#define LOG_NOTICE(format, args...) pr_notice(MyTag format,  ##args)
#define LOG_WRN(format, args...)    pr_warn(MyTag format,  ##args)
#define LOG_ERR(format, args...)    pr_err(MyTag format,  ##args)
#define LOG_AST(format, args...)    pr_alert(MyTag format, ##args)


/*******************************************************************************
* defined marco
********************************************************************************/

/* #define ISP_WR32(addr, data)    iowrite32(data, addr)	*/ /* For other projects. */
/* #define ISP_WR32(addr, data)    mt65xx_reg_sync_writel(data, addr)    // For 89 Only.NEED_TUNING_BY_PROJECT */
/* #define ISP_RD32(addr)          ioread32(addr) */
#define ISP_SET_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) |= (MUINT32)(1 << (bit)))
#define ISP_CLR_BIT(reg, bit)   ((*(volatile MUINT32*)(reg)) &= ~((MUINT32)(1 << (bit))))

#define OVERRUN_AEE_WARNING

/*******************************************************************************
* defined value
********************************************************************************/
/* ///////////////////////////////////////////////////////////////// */
/* for restricting range in mmap function */
/* isp driver */
#define ISP_RTBUF_REG_RANGE  0x10000
#define IMGSYS_BASE_ADDR     0x15000000
#define ISP_REG_RANGE           (0x10000)	/* 0x6100,the same with the value in isp_reg.h and page-aligned */
/* seninf driver */
#define SENINF_BASE_ADDR     0x15008000	/* the same with the value in seninf_drv.cpp(chip-dependent) */
#define SENINF_REG_RANGE    (0x4000)	/* 0x800,the same with the value in seninf_reg.h and page-aligned */
/* #define IMGSYS_CG_CLR0_ADDR  0x15000000 //the same with the value in seninf_drv.cpp(chip-dependent) */
/* #define MMSYS_RANGE           (0x1000)   //0x100,the same with the value in seninf_drv.cpp and page-aligned */
#define PLL_BASE_ADDR        0x10000000	/* the same with the value in seninf_drv.cpp(chip-dependent) */
#define PLL_RANGE            (0x1000)	/* 0x200,the same with the value in seninf_drv.cpp and page-aligned */
#define MIPIRX_CONFIG_ADDR   0x1500C000	/* the same with the value in seninf_drv.cpp(chip-dependent) */
#define MIPIRX_CONFIG_RANGE (0x100)	/* 0x100,the same with the value in seninf_drv.cpp and page-aligned */
#define MIPIRX_ANALOG_ADDR   0x10215000	/* the same with the value in seninf_drv.cpp(chip-dependent) */
#define MIPIRX_ANALOG_RANGE (0x3000)
#define GPIO_BASE_ADDR       0x10211000	/* the same with the value in seninf_drv.cpp(chip-dependent) */
#define GPIO_RANGE          (0x1000)
#define EFUSE_BASE_ADDR      0x10206000
#define EFUSE_RANGE          (0x1000)	/* 0x400,the same with the value in seninf_drv.cpp and page-aligned */
/* security concern */
#define ISP_RANGE         (0x10000)
/* ///////////////////////////////////////////////////////////////// */


#define ISP_DEV_NAME                "camera-isp"
#define ISP_IRQ_POLLING             (0)

#define ISP_DBG_INT                 (0x00000001)
#define ISP_DBG_HOLD_REG            (0x00000002)
#define ISP_DBG_READ_REG            (0x00000004)
#define ISP_DBG_WRITE_REG           (0x00000008)
#define ISP_DBG_CLK                 (0x00000010)
#define ISP_DBG_TASKLET             (0x00000020)
#define ISP_DBG_SCHEDULE_WORK       (0x00000040)
#define ISP_DBG_BUF_WRITE           (0x00000080)
#define ISP_DBG_BUF_CTRL            (0x00000100)
#define ISP_DBG_REF_CNT_CTRL        (0x00000200)

#ifdef CONFIG_OF

static unsigned long gISPSYS_Irq[ISP_CAM_IRQ_IDX_NUM];
static unsigned long gISPSYS_Reg[ISP_CAM_BASEADDR_NUM];
#undef ISP_ADDR
#undef ISP_IMGSYS_BASE
#undef ISP_ADDR_CAMINF
#undef ISP_MIPI_ANA_ADDR
#undef ISP_IMGSYS_BASE_PHY
#define ISP_ADDR                        (gISPSYS_Reg[ISP_BASE_ADDR])
#define ISP_IMGSYS_BASE                 (gISPSYS_Reg[ISP_IMGSYS_CONFIG_BASE_ADDR])
#define ISP_ADDR_CAMINF                 (gISPSYS_Reg[ISP_IMGSYS_CONFIG_BASE_ADDR])
#define ISP_MIPI_ANA_ADDR               (gISPSYS_Reg[ISP_MIPI_ANA_BASE_ADDR])
#define ISP_IMGSYS_BASE_PHY             0x15000000
#else
#undef ISP_ADDR
#undef ISP_ADDR_CAMINF
#undef ISP_MIPI_ANA_ADDR
#define ISP_ADDR                        (CAMINF_BASE + 0x4000)
#define ISP_ADDR_CAMINF                 CAMINF_BASE
#define ISP_MIPI_ANA_ADDR               0x10215000
#endif


#define ISP_REG_ADDR_EN1                (ISP_ADDR + 0x4)
#define ISP_REG_ADDR_INT_STATUS         (ISP_ADDR + 0x24)
#define ISP_REG_ADDR_DMA_INT            (ISP_ADDR + 0x28)
#define ISP_REG_ADDR_INTB_STATUS        (ISP_ADDR + 0x30)
#define ISP_REG_ADDR_DMAB_INT           (ISP_ADDR + 0x34)
#define ISP_REG_ADDR_INTC_STATUS        (ISP_ADDR + 0x3C)
#define ISP_REG_ADDR_DMAC_INT           (ISP_ADDR + 0x40)
#define ISP_REG_ADDR_INT_STATUSX        (ISP_ADDR + 0x44)
#define ISP_REG_ADDR_DMA_INTX           (ISP_ADDR + 0x48)
#define ISP_REG_ADDR_SW_CTL             (ISP_ADDR + 0x5C)
#define ISP_REG_ADDR_CQ0C_BASE_ARRR     (ISP_ADDR + 0xBC)
#define ISP_REG_ADDR_IMGO_FBC           (ISP_ADDR + 0xF4)
#define ISP_REG_ADDR_IMG2O_FBC          (ISP_ADDR + 0xF8)
#define ISP_REG_ADDR_FBC_INT            (ISP_ADDR + 0xFC)
#define ISP_REG_ADDR_IMGO_BASE_ADDR     (ISP_ADDR + 0x300)
#define ISP_REG_ADDR_IMG2O_BASE_ADDR    (ISP_ADDR + 0x320)
#define ISP_REG_ADDR_TG_VF_CON          (ISP_ADDR + 0x414)
#define ISP_REG_ADDR_CTL_DBG_SET         (ISP_ADDR + 0x160)
#define ISP_REG_ADDR_CTL_DBG_PORT        (ISP_ADDR + 0x164)
#define ISP_REG_ADDR_CTL_EN2        (ISP_ADDR + 0x008)
#define ISP_REG_ADDR_CTL_CROP_X        (ISP_ADDR + 0x110)

#define ISP_REG_ADDR_CTL_DBG_SET_CQ_STS                  (0x6000)

#define ISP_REG_ADDR_CTL_DBG_SET_IMGI_STS                  (0x9003)
#define ISP_REG_ADDR_CTL_DBG_SET_IMGI_SYNC                (0x9004)
#define ISP_REG_ADDR_CTL_DBG_SET_IMGI_NO_SYNC          (0x9005)

#define ISP_REG_ADDR_CTL_DBG_SET_CFA_STS                  (0x9006)
#define ISP_REG_ADDR_CTL_DBG_SET_CFA_SYNC                (0x9007)
#define ISP_REG_ADDR_CTL_DBG_SET_CFA_NO_SYNC          (0x9008)

#define ISP_REG_ADDR_CTL_DBG_SET_YUV_STS                  (0x9009)
#define ISP_REG_ADDR_CTL_DBG_SET_YUV_SYNC                (0x900a)
#define ISP_REG_ADDR_CTL_DBG_SET_YUV_NO_SYNC          (0x900b)

#define ISP_REG_ADDR_CTL_DBG_SET_OUT_STS                  (0x900c)
#define ISP_REG_ADDR_CTL_DBG_SET_OUT_SYNC                (0x900d)
#define ISP_REG_ADDR_CTL_DBG_SET_OUT_NO_SYNC          (0x900e)

#define ISP_REG_ADDR_CTL_EN2_UV_CRSA_EN_BIT        (1<<23)
#define ISP_REG_ADDR_CTL_CROP_X_MDP_CROP_EN_BIT        (1<<15)

/* #define ISP_REG_ADDR_TG2_VF_CON         (ISP_ADDR + 0x4B4) */


#define ISP_TPIPE_ADDR                  (0x15004000)

/* ====== CAM_CTL_SW_CTL ====== */

#define ISP_REG_SW_CTL_SW_RST_TRIG      (0x00000001)
#define ISP_REG_SW_CTL_SW_RST_STATUS    (0x00000002)
#define ISP_REG_SW_CTL_HW_RST           (0x00000004)

/* ====== CAM_CTL_INT_STATUS ====== */

#define ISP_REG_MASK_INT_STATUS         (ISP_IRQ_INT_STATUS_VS1_ST |\
					    ISP_IRQ_INT_STATUS_TG1_ST1 |\
					    ISP_IRQ_INT_STATUS_TG1_ST2 |\
					    ISP_IRQ_INT_STATUS_EXPDON1_ST |\
					    ISP_IRQ_INT_STATUS_PASS1_TG1_DON_ST |\
					    ISP_IRQ_INT_STATUS_SOF1_INT_ST |\
					    ISP_IRQ_INT_STATUS_PASS2_DON_ST |\
					    ISP_IRQ_INT_STATUS_TPIPE_DON_ST |\
					    ISP_IRQ_INT_STATUS_AF_DON_ST |\
					    ISP_IRQ_INT_STATUS_FLK_DON_ST |\
					    ISP_IRQ_INT_STATUS_CQ_DON_ST)

/* IRQ Error Mask */
#define ISP_REG_MASK_INT_STATUS_ERR     (ISP_IRQ_INT_STATUS_TG1_ERR_ST |\
					    ISP_IRQ_INT_STATUS_CQ_ERR_ST |\
					    ISP_IRQ_INT_STATUS_IMGO_ERR_ST |\
					    ISP_IRQ_INT_STATUS_AAO_ERR_ST |\
					    ISP_IRQ_INT_STATUS_IMG2O_ERR_ST |\
					    ISP_IRQ_INT_STATUS_ESFKO_ERR_ST |\
					    ISP_IRQ_INT_STATUS_FLK_ERR_ST |\
					    ISP_IRQ_INT_STATUS_LSC_ERR_ST |\
					    ISP_IRQ_INT_STATUS_DMA_ERR_ST)


/* ====== CAM_CTL_DMA_INT ====== */

/* IRQ  Mask */
#define ISP_REG_MASK_DMA_INT            (ISP_IRQ_DMA_INT_IMGO_DONE_ST |\
					    ISP_IRQ_DMA_INT_IMG2O_DONE_ST |\
					    ISP_IRQ_DMA_INT_AAO_DONE_ST |\
					    ISP_IRQ_DMA_INT_ESFKO_DONE_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_DMA_INT_ERR        (ISP_IRQ_DMA_INT_CQ0_ERR_ST |\
					    ISP_IRQ_DMA_INT_TG1_GBERR_ST)

/* ====== CAM_CTL_INTB_STATUS ====== */

/* IRQ  Mask */
#define ISP_REG_MASK_INTB_STATUS        (ISP_IRQ_INTB_STATUS_PASS2_DON_ST |\
					    ISP_IRQ_INTB_STATUS_TPIPE_DON_ST |\
					    ISP_IRQ_INTB_STATUS_CQ_DON_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_INTB_STATUS_ERR    (ISP_IRQ_INTB_STATUS_CQ_ERR_ST |\
					    ISP_IRQ_INTB_STATUS_IMGO_ERR_ST |\
					    ISP_IRQ_INTB_STATUS_LCSO_ERR_ST |\
					    ISP_IRQ_INTB_STATUS_IMG2O_ERR_ST |\
					    ISP_IRQ_INTB_STATUS_LSC_ERR_ST |\
					    ISP_IRQ_INTB_STATUS_LCE_ERR_ST |\
					    ISP_IRQ_INTB_STATUS_DMA_ERR_ST)
/* CAM_CTL_DMAB_INT */
/* IRQ  Mask */
#define ISP_REG_MASK_DMAB_INT           (ISP_IRQ_DMAB_INT_IMGO_DONE_ST |\
					    ISP_IRQ_DMAB_INT_IMG2O_DONE_ST |\
					    ISP_IRQ_DMAB_INT_AAO_DONE_ST |\
					    ISP_IRQ_DMAB_INT_LCSO_DONE_ST |\
					    ISP_IRQ_DMAB_INT_ESFKO_DONE_ST |\
					    ISP_IRQ_DMAB_INT_DISPO_DONE_ST |\
					    ISP_IRQ_DMAB_INT_VIDO_DONE_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_DMAB_INT_ERR	/* (   ISP_IRQ_DMAB_INT_NR3O_ERR_ST) */

/* ====== CAM_CTL_INTC_STATUS ====== */

/* IRQ  Mask */
#define ISP_REG_MASK_INTC_STATUS        (ISP_IRQ_INTC_STATUS_PASS2_DON_ST |\
					    ISP_IRQ_INTC_STATUS_TPIPE_DON_ST |\
					    ISP_IRQ_INTC_STATUS_CQ_DON_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_INTC_STATUS_ERR    (ISP_IRQ_INTC_STATUS_CQ_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_IMGO_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_LCSO_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_IMG2O_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_LSC_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_BPC_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_LCE_ERR_ST |\
					    ISP_IRQ_INTC_STATUS_DMA_ERR_ST)
/* ====== CAM_CTL_DMAC_INT ====== */

/* IRQ  Mask */
#define ISP_REG_MASK_DMAC_INT           (ISP_IRQ_DMAC_INT_IMGO_DONE_ST |\
					    ISP_IRQ_DMAC_INT_IMG2O_DONE_ST |\
					    ISP_IRQ_DMAC_INT_AAO_DONE_ST |\
					    ISP_IRQ_DMAC_INT_LCSO_DONE_ST |\
					    ISP_IRQ_DMAC_INT_ESFKO_DONE_ST |\
					    ISP_IRQ_DMAC_INT_DISPO_DONE_ST |\
					    ISP_IRQ_DMAC_INT_VIDO_DONE_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_DMAC_INT_ERR	/* (   ISP_IRQ_DMAC_INT_NR3O_ERR_ST) */

/* ====== CAM_CTL_INT_STATUSX ====== */

/* IRQ  Mask */
#define ISP_REG_MASK_INTX_STATUS        (ISP_IRQ_INTX_STATUS_VS1_ST |\
					    ISP_IRQ_INTX_STATUS_TG1_ST1 |\
					    ISP_IRQ_INTX_STATUS_TG1_ST2 |\
					    ISP_IRQ_INTX_STATUS_EXPDON1_ST |\
					    ISP_IRQ_INTX_STATUS_VS2_ST |\
					    ISP_IRQ_INTX_STATUS_TG2_ST1 |\
					    ISP_IRQ_INTX_STATUS_TG2_ST2 |\
					    ISP_IRQ_INTX_STATUS_EXPDON2_ST |\
					    ISP_IRQ_INTX_STATUS_PASS1_TG1_DON_ST |\
					    ISP_IRQ_INTX_STATUS_PASS1_TG2_DON_ST |\
					    ISP_IRQ_INTX_STATUS_PASS2_DON_ST |\
					    ISP_IRQ_INTX_STATUS_TPIPE_DON_ST |\
					    ISP_IRQ_INTX_STATUS_AF_DON_ST |\
					    ISP_IRQ_INTX_STATUS_FLK_DON_ST |\
					    ISP_IRQ_INTX_STATUS_FMT_DON_ST |\
					    ISP_IRQ_INTX_STATUS_CQ_DON_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_INTX_STATUS_ERR    (ISP_IRQ_INTX_STATUS_TG1_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_TG2_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_CQ_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_IMGO_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_AAO_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_LCSO_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_LCSO_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_ESFKO_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_FLK_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_LSC_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_LCE_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_DMA_ERR_ST)
/* ISP_IRQ_INTX_STATUS_BPC_ERR_ST    //Vent@20121025: Remove ISP_IRQ_INTX_STATUS_BPC_ERR_ST. From TH Wu's explanation, this bit is not used as an error state anymore. */

#define ISP_REG_MASK_INTX_STATUS_ERR_FRMB (ISP_IRQ_INTX_STATUS_TG1_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_CQ_ERR_ST |\
					    ISP_IRQ_INTX_STATUS_DMA_ERR_ST)



/* ====== CAM_CTL_DMA_INTX ====== */

/* IRQ  Mask */
#define ISP_REG_MASK_DMAX_INT           (ISP_IRQ_DMAX_INT_IMGO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_IMG2O_DONE_ST |\
					    ISP_IRQ_DMAX_INT_AAO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_LCSO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_ESFKO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_DISPO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_VIDO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_VRZO_DONE_ST |\
					    ISP_IRQ_DMAX_INT_NR3O_DONE_ST |\
					    ISP_IRQ_DMAX_INT_BUF_OVL_ST)
/* IRQ Error Mask */
#define ISP_REG_MASK_DMAX_INT_ERR       (ISP_IRQ_DMAX_INT_NR3O_ERR_ST |\
					    ISP_IRQ_DMAX_INT_CQ_ERR_ST |\
					    ISP_IRQ_DMAX_INT_TG1_GBERR_ST |\
					    ISP_IRQ_DMAX_INT_TG2_GBERR_ST)

/*******************************************************************************
* variable/lock for hal version judgement
********************************************************************************/
/* 0 means camera 1, and 1 for camera 3 */
static bool CAM_HAL_VER_IS3;
static spinlock_t SpinLockCamHaVer; /* remove volatile */


int pr_detect_count;

/*******************************************************************************
* struct & enum
********************************************************************************/

#define ISP_BUF_SIZE            (4096)
#define ISP_BUF_SIZE_WRITE      1024
#define ISP_BUF_WRITE_AMOUNT    6

typedef enum {
	ISP_BUF_STATUS_EMPTY,
	ISP_BUF_STATUS_HOLD,
	ISP_BUF_STATUS_READY
} ISP_BUF_STATUS_ENUM;

typedef struct {
	pid_t Pid;
	pid_t Tid;
} ISP_USER_INFO_STRUCT;

typedef struct {
	volatile ISP_BUF_STATUS_ENUM Status;
	volatile MUINT32 Size;
	MUINT8 *pData;
} ISP_BUF_STRUCT;

typedef struct {
	ISP_BUF_STRUCT Read;
	ISP_BUF_STRUCT Write[ISP_BUF_WRITE_AMOUNT];
} ISP_BUF_INFO_STRUCT;

typedef struct {
	atomic_t HoldEnable;
	atomic_t WriteEnable;
	ISP_HOLD_TIME_ENUM Time;
} ISP_HOLD_INFO_STRUCT;

typedef struct {
	MUINT32 Status[ISP_IRQ_TYPE_AMOUNT];
	MUINT32 Mask[ISP_IRQ_TYPE_AMOUNT];
	MUINT32 ErrMask[ISP_IRQ_TYPE_AMOUNT];
} ISP_IRQ_INFO_STRUCT;

typedef struct {
	MUINT32 Vd;
	MUINT32 Expdone;
	MUINT32 WorkQueueVd;
	MUINT32 WorkQueueExpdone;
	MUINT32 WorkQueueSeninf;
	MUINT32 TaskletVd;
	MUINT32 TaskletExpdone;
	MUINT32 TaskletSeninf;
} ISP_TIME_LOG_STRUCT;

typedef struct {
	spinlock_t SpinLockIspRef;
	spinlock_t SpinLockIsp;
	spinlock_t SpinLockIrq;
	spinlock_t SpinLockHold;
	spinlock_t SpinLockRTBC;
	wait_queue_head_t WaitQueueHead;
	struct work_struct ScheduleWorkVD;
	struct work_struct ScheduleWorkEXPDONE;
	struct work_struct ScheduleWorkSENINF;
	MUINT32 UserCount;
	MUINT32 DebugMask;
	MINT32 IrqNum;
	ISP_IRQ_INFO_STRUCT IrqInfo;
	ISP_HOLD_INFO_STRUCT HoldInfo;
	ISP_BUF_INFO_STRUCT BufInfo;
	ISP_TIME_LOG_STRUCT TimeLog;
	ISP_CALLBACK_STRUCT Callback[ISP_CALLBACK_AMOUNT];
} ISP_INFO_STRUCT;

/*checkpatch.pl bug ?? WARNING:AVOID_EXTERNS: externs should be avoided in .c files */
/*m4u_callback_ret_t ISP_M4U_TranslationFault_callback(int port, unsigned int mva, void *data);*/

/*******************************************************************************
* internal global data
********************************************************************************/

/* pointer to the kmalloc'd area, rounded up to a page boundary */
static MINT32 *g_pTbl_RTBuf;

/* original pointer for kmalloc'd area as returned by kmalloc */
static MVOID *g_pBuf_kmalloc;

static ISP_RT_BUF_STRUCT *g_pstRTBuf;

static ISP_INFO_STRUCT g_IspInfo;

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source isp_wake_lock;
#else
struct wake_lock isp_wake_lock;
#endif

#if (LOG_CONSTRAINT_ADJ == 1)
static MBOOL g_log_def_constraint = 100;
#endif

static volatile int g_bWaitLock;

static atomic_t g_imem_ref_cnt[ISP_REF_CNT_ID_MAX];

MUINT32 g_EnableClkCnt = 0;
volatile MUINT32 g_TempAddr = 0;

/* static ISP_DEQUE_BUF_INFO_STRUCT g_deque_buf = {0,{}};    // Marked to remove build warning.WARNING */

unsigned long g_Flash_SpinLock;
static MINT32 mMclk1User;

#ifndef _rtbc_use_cq0c_
static MUINT32 g_rtbcAAA;
static MUINT32 g_EnqBuf;
static MUINT32 g_DeqBuf;
static MINT32 g_rtbc_enq_dma = _rt_dma_max_;
static MINT32 g_rtbc_deq_dma = _rt_dma_max_;
#endif

static ISP_RT_BUF_INFO_STRUCT g_rt_buf_info;
static ISP_RT_BUF_INFO_STRUCT g_ex_rt_buf_info;
static ISP_DEQUE_BUF_INFO_STRUCT g_deque_buf;

static MUINT32 g_prv_tstamp_s;
static MUINT32 g_prv_tstamp_us;

static MUINT32 g_sof_count;
static MUINT32 g_start_time;
static MUINT32 g_avg_frame_time;

/* allan */
static MINT32 g_sof_pass1done;

static dev_t g_IspDevNo;
static struct cdev *g_pIspCharDrv;
static struct class *g_pIspClass;

static MINT32 g_bPass1_On_In_Resume_TG1;

#define DEFAULT_PA 0x3773

static volatile MUINT32 g_oldImgoAddr = DEFAULT_PA;
static volatile MUINT32 g_newImgoAddr = DEFAULT_PA;
static volatile MUINT32 g_oldImg2oAddr = DEFAULT_PA;
static volatile MUINT32 g_newImg2oAddr = DEFAULT_PA;

typedef struct {
	MUINT32 regVal_1;
	MUINT32 regVal_2;
} SENINF_DEBUG;

static SENINF_DEBUG g_seninfDebug[30];

/*******************************************************************************
*
********************************************************************************/

/* test flag */
#define ISP_KERNEL_MOTIFY_SINGAL_TEST
#ifdef ISP_KERNEL_MOTIFY_SINGAL_TEST
/*** Linux signal test ***/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/siginfo.h>	/* siginfo */
#include <linux/rcupdate.h>	/* rcu_read_lock */
#include <linux/sched.h>	/* find_task_by_pid_type */
#include <linux/debugfs.h>
#include <linux/uaccess.h>

/* js_test */
#define __tcmfunc


#define SIG_TEST 44		/* we choose 44 as our signal number (real-time signals are in the range of 33 to 64) */

struct siginfo info;
struct task_struct *t;


int getTaskInfo(pid_t pid)
{
	/* send the signal */
	memset(&info, 0, sizeof(struct siginfo));
	info.si_signo = SIG_TEST;
	info.si_code = SI_QUEUE;	/* this is bit of a trickery: SI_QUEUE is normally used by sigqueue from user space, */
	/* and kernel space should use SI_KERNEL. But if SI_KERNEL is used the real_time data */
	/* is not delivered to the user space signal handler function. */
	info.si_int = 1234;	/* real time signals may have 32 bits of data. */

	rcu_read_lock();

	t = find_task_by_vpid(pid);
	/* t = find_task_by_pid_type(PIDTYPE_PID, g_pid);  //find the task_struct associated with this pid */
	if (t == NULL) {
		LOG_DBG("no such pid");
		rcu_read_unlock();
		return -ENODEV;
	}
	rcu_read_unlock();

	return 0;
}

int sendSignal(void)
{
	int ret = 0;

	ret = send_sig_info(SIG_TEST, &info, t);	/* send the signal */
	if (ret < 0) {
		LOG_DBG("error sending signal");
		return ret;
	}

	return ret;
}

/*** Linux signal test ***/

#endif				/* ISP_KERNEL_MOTIFY_SINGAL_TEST */

/*******************************************************************************
* transfer ms to jiffies
********************************************************************************/
static inline MUINT32 ISP_MsToJiffies(MUINT32 Ms)
{
	return (Ms * HZ + 512) >> 10;
}

/*******************************************************************************
* transfer jiffies to ms
********************************************************************************/
static inline MUINT32 ISP_JiffiesToMs(MUINT32 Jiffies)
{
	return (Jiffies * 1000) / HZ;
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 ISP_GetIRQState(MUINT32 type, MUINT32 stus)
{
	MUINT32 ret;
	unsigned long flags;

	spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);
	ret = (g_IspInfo.IrqInfo.Status[type] & stus);
	spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);

	return ret;
}

/*******************************************************************************
* get kernel time
********************************************************************************/
static void ISP_GetTime(MUINT32 *pSec, MUINT32 *pUSec)
{
	ktime_t Time;
	MUINT64 TimeSec;

	Time = ktime_get();	/* ns */
	TimeSec = Time.tv64;
	do_div(TimeSec, 1000);
	*pUSec = do_div(TimeSec, 1000000);
	*pSec = (MUINT64) TimeSec;
}

#define RegDump(start, end) {\
	MUINT32 i;\
	for (i = start; i <= end; i += 0x4) {\
		LOG_DBG("0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + i), (unsigned int)ISP_RD32(ISP_ADDR + i));\
	} \
}
/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_DumpReg(MVOID)
{
	MINT32 Ret = 0;
	MINT32 i;

	LOG_DBG("+");

	/* spin_lock_irqsave(&(g_IspInfo.SpinLock), flags); */

	/* tile tool parse range */
	/* Joseph Hung (xa)#define ISP_ADDR_START  0x15004000 */
	/* #define ISP_ADDR_END    0x15006000 */
	/*  */


	/* for tpipe main start */
	LOG_DBG("start MT");
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x000, ISP_RD32((void *)(ISP_ADDR + 0x000)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x004, ISP_RD32((void *)(ISP_ADDR + 0x004)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x008, ISP_RD32((void *)(ISP_ADDR + 0x008)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x00C, ISP_RD32((void *)(ISP_ADDR + 0x00C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x010, ISP_RD32((void *)(ISP_ADDR + 0x010)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x018, ISP_RD32((void *)(ISP_ADDR + 0x018)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x01C, ISP_RD32((void *)(ISP_ADDR + 0x01C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x020, ISP_RD32((void *)(ISP_ADDR + 0x020)));

#if 1				/* it may touch ReadClear register */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x024, ISP_RD32((void *)(ISP_ADDR + 0x024)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x028, ISP_RD32((void *)(ISP_ADDR + 0x028)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x02C, ISP_RD32((void *)(ISP_ADDR + 0x02C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x030, ISP_RD32((void *)(ISP_ADDR + 0x030)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x034, ISP_RD32((void *)(ISP_ADDR + 0x034)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x038, ISP_RD32((void *)(ISP_ADDR + 0x038)));
#else
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x024, 0);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x028, 0);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x02C, 0);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x030, 0);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x034, 0);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x038, 0);
#endif

	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x050, ISP_RD32((void *)(ISP_ADDR + 0x050)));	/* Tile setting */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x074, ISP_RD32((void *)(ISP_ADDR + 0x074)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x078, ISP_RD32((void *)(ISP_ADDR + 0x078)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x07C, ISP_RD32((void *)(ISP_ADDR + 0x07C)));
#if 0				/* WO */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x080, ISP_RD32((void *)(ISP_ADDR + 0x080)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x084, ISP_RD32((void *)(ISP_ADDR + 0x084)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x088, ISP_RD32((void *)(ISP_ADDR + 0x088)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x08C, ISP_RD32((void *)(ISP_ADDR + 0x08C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x090, ISP_RD32((void *)(ISP_ADDR + 0x090)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x094, ISP_RD32((void *)(ISP_ADDR + 0x094)));
#endif
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0A8, ISP_RD32((void *)(ISP_ADDR + 0x0A8)));	/* cq0 base */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0AC, ISP_RD32((void *)(ISP_ADDR + 0x0AC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0B0, ISP_RD32((void *)(ISP_ADDR + 0x0B0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0B4, ISP_RD32((void *)(ISP_ADDR + 0x0B4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0B8, ISP_RD32((void *)(ISP_ADDR + 0x0B8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0BC, ISP_RD32((void *)(ISP_ADDR + 0x0BC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0E8, ISP_RD32((void *)(ISP_ADDR + 0x0E8)));	/* cq0 cur base */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0F4, ISP_RD32((void *)(ISP_ADDR + 0x0F4)));	/* fbc imgo */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x0F8, ISP_RD32((void *)(ISP_ADDR + 0x0F8)));	/* fbc img2o */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x110, ISP_RD32((void *)(ISP_ADDR + 0x110)));	/* mdp crop x */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x114, ISP_RD32((void *)(ISP_ADDR + 0x114)));	/* mdp crop y */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x138, ISP_RD32((void *)(ISP_ADDR + 0x138)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x13C, ISP_RD32((void *)(ISP_ADDR + 0x13C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x144, ISP_RD32((void *)(ISP_ADDR + 0x144)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x148, ISP_RD32((void *)(ISP_ADDR + 0x148)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x14C, ISP_RD32((void *)(ISP_ADDR + 0x14C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x204, ISP_RD32((void *)(ISP_ADDR + 0x204)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x20C, ISP_RD32((void *)(ISP_ADDR + 0x20C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x22C, ISP_RD32((void *)(ISP_ADDR + 0x22C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x230, ISP_RD32((void *)(ISP_ADDR + 0x230)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x234, ISP_RD32((void *)(ISP_ADDR + 0x234)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x238, ISP_RD32((void *)(ISP_ADDR + 0x238)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x23C, ISP_RD32((void *)(ISP_ADDR + 0x23C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x240, ISP_RD32((void *)(ISP_ADDR + 0x240)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x244, ISP_RD32((void *)(ISP_ADDR + 0x244)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x258, ISP_RD32((void *)(ISP_ADDR + 0x258)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x260, ISP_RD32((void *)(ISP_ADDR + 0x260)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x26C, ISP_RD32((void *)(ISP_ADDR + 0x26C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x270, ISP_RD32((void *)(ISP_ADDR + 0x270)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x27C, ISP_RD32((void *)(ISP_ADDR + 0x27C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x298, ISP_RD32((void *)(ISP_ADDR + 0x298)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2B4, ISP_RD32((void *)(ISP_ADDR + 0x2B4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2D0, ISP_RD32((void *)(ISP_ADDR + 0x2D0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2D4, ISP_RD32((void *)(ISP_ADDR + 0x2D4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2F0, ISP_RD32((void *)(ISP_ADDR + 0x2F0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x2F4, ISP_RD32((void *)(ISP_ADDR + 0x2F4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x300, ISP_RD32((void *)(ISP_ADDR + 0x300)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x304, ISP_RD32((void *)(ISP_ADDR + 0x304)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x308, ISP_RD32((void *)(ISP_ADDR + 0x308)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x30C, ISP_RD32((void *)(ISP_ADDR + 0x30C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x310, ISP_RD32((void *)(ISP_ADDR + 0x310)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x314, ISP_RD32((void *)(ISP_ADDR + 0x314)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x318, ISP_RD32((void *)(ISP_ADDR + 0x318)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x31C, ISP_RD32((void *)(ISP_ADDR + 0x31C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x320, ISP_RD32((void *)(ISP_ADDR + 0x320)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x324, ISP_RD32((void *)(ISP_ADDR + 0x324)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x328, ISP_RD32((void *)(ISP_ADDR + 0x328)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x32C, ISP_RD32((void *)(ISP_ADDR + 0x32C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x330, ISP_RD32((void *)(ISP_ADDR + 0x330)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x334, ISP_RD32((void *)(ISP_ADDR + 0x334)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x338, ISP_RD32((void *)(ISP_ADDR + 0x338)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x33C, ISP_RD32((void *)(ISP_ADDR + 0x33C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x350, ISP_RD32((void *)(ISP_ADDR + 0x350)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x37C, ISP_RD32((void *)(ISP_ADDR + 0x37C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x398, ISP_RD32((void *)(ISP_ADDR + 0x398)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x410, ISP_RD32((void *)(ISP_ADDR + 0x410)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x414, ISP_RD32((void *)(ISP_ADDR + 0x414)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x418, ISP_RD32((void *)(ISP_ADDR + 0x418)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x41C, ISP_RD32((void *)(ISP_ADDR + 0x41C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x420, ISP_RD32((void *)(ISP_ADDR + 0x420)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x448, ISP_RD32((void *)(ISP_ADDR + 0x448)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4B0, ISP_RD32((void *)(ISP_ADDR + 0x4B0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4B4, ISP_RD32((void *)(ISP_ADDR + 0x4B4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4B8, ISP_RD32((void *)(ISP_ADDR + 0x4B8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4BC, ISP_RD32((void *)(ISP_ADDR + 0x4BC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4E8, ISP_RD32((void *)(ISP_ADDR + 0x4E8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x534, ISP_RD32((void *)(ISP_ADDR + 0x534)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x538, ISP_RD32((void *)(ISP_ADDR + 0x538)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x53C, ISP_RD32((void *)(ISP_ADDR + 0x53C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x580, ISP_RD32((void *)(ISP_ADDR + 0x580)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x584, ISP_RD32((void *)(ISP_ADDR + 0x584)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x800, ISP_RD32((void *)(ISP_ADDR + 0x800)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x880, ISP_RD32((void *)(ISP_ADDR + 0x880)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x884, ISP_RD32((void *)(ISP_ADDR + 0x884)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x888, ISP_RD32((void *)(ISP_ADDR + 0x888)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x8A0, ISP_RD32((void *)(ISP_ADDR + 0x8A0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x920, ISP_RD32((void *)(ISP_ADDR + 0x920)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x924, ISP_RD32((void *)(ISP_ADDR + 0x924)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x928, ISP_RD32((void *)(ISP_ADDR + 0x928)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x92C, ISP_RD32((void *)(ISP_ADDR + 0x92C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x930, ISP_RD32((void *)(ISP_ADDR + 0x930)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x934, ISP_RD32((void *)(ISP_ADDR + 0x934)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x938, ISP_RD32((void *)(ISP_ADDR + 0x938)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x93C, ISP_RD32((void *)(ISP_ADDR + 0x93C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x948, ISP_RD32((void *)(ISP_ADDR + 0x948)));	/* unp offset */
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x960, ISP_RD32((void *)(ISP_ADDR + 0x960)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9C4, ISP_RD32((void *)(ISP_ADDR + 0x9C4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9E4, ISP_RD32((void *)(ISP_ADDR + 0x9E4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9E8, ISP_RD32((void *)(ISP_ADDR + 0x9E8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x9EC, ISP_RD32((void *)(ISP_ADDR + 0x9EC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA00, ISP_RD32((void *)(ISP_ADDR + 0xA00)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA04, ISP_RD32((void *)(ISP_ADDR + 0xA04)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA08, ISP_RD32((void *)(ISP_ADDR + 0xA08)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA0C, ISP_RD32((void *)(ISP_ADDR + 0xA0C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA10, ISP_RD32((void *)(ISP_ADDR + 0xA10)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA14, ISP_RD32((void *)(ISP_ADDR + 0xA14)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xA20, ISP_RD32((void *)(ISP_ADDR + 0xA20)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xAA0, ISP_RD32((void *)(ISP_ADDR + 0xAA0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xACC, ISP_RD32((void *)(ISP_ADDR + 0xACC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB00, ISP_RD32((void *)(ISP_ADDR + 0xB00)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB04, ISP_RD32((void *)(ISP_ADDR + 0xB04)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB08, ISP_RD32((void *)(ISP_ADDR + 0xB08)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB0C, ISP_RD32((void *)(ISP_ADDR + 0xB0C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB10, ISP_RD32((void *)(ISP_ADDR + 0xB10)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB14, ISP_RD32((void *)(ISP_ADDR + 0xB14)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB18, ISP_RD32((void *)(ISP_ADDR + 0xB18)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB1C, ISP_RD32((void *)(ISP_ADDR + 0xB1C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB20, ISP_RD32((void *)(ISP_ADDR + 0xB20)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB44, ISP_RD32((void *)(ISP_ADDR + 0xB44)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB48, ISP_RD32((void *)(ISP_ADDR + 0xB48)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB4C, ISP_RD32((void *)(ISP_ADDR + 0xB4C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB50, ISP_RD32((void *)(ISP_ADDR + 0xB50)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB54, ISP_RD32((void *)(ISP_ADDR + 0xB54)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB58, ISP_RD32((void *)(ISP_ADDR + 0xB58)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB5C, ISP_RD32((void *)(ISP_ADDR + 0xB5C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xB60, ISP_RD32((void *)(ISP_ADDR + 0xB60)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBA0, ISP_RD32((void *)(ISP_ADDR + 0xBA0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBA4, ISP_RD32((void *)(ISP_ADDR + 0xBA4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBA8, ISP_RD32((void *)(ISP_ADDR + 0xBA8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBAC, ISP_RD32((void *)(ISP_ADDR + 0xBAC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBB0, ISP_RD32((void *)(ISP_ADDR + 0xBB0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBB4, ISP_RD32((void *)(ISP_ADDR + 0xBB4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBB8, ISP_RD32((void *)(ISP_ADDR + 0xBB8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBBC, ISP_RD32((void *)(ISP_ADDR + 0xBBC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xBC0, ISP_RD32((void *)(ISP_ADDR + 0xBC0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xC20, ISP_RD32((void *)(ISP_ADDR + 0xC20)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCC0, ISP_RD32((void *)(ISP_ADDR + 0xCC0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCE4, ISP_RD32((void *)(ISP_ADDR + 0xCE4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCE8, ISP_RD32((void *)(ISP_ADDR + 0xCE8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCEC, ISP_RD32((void *)(ISP_ADDR + 0xCEC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCF0, ISP_RD32((void *)(ISP_ADDR + 0xCF0)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCF4, ISP_RD32((void *)(ISP_ADDR + 0xCF4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCF8, ISP_RD32((void *)(ISP_ADDR + 0xCF8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xCFC, ISP_RD32((void *)(ISP_ADDR + 0xCFC)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD24, ISP_RD32((void *)(ISP_ADDR + 0xD24)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD28, ISP_RD32((void *)(ISP_ADDR + 0xD28)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD2C, ISP_RD32((void *)(ISP_ADDR + 0xD2c)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD40, ISP_RD32((void *)(ISP_ADDR + 0xD40)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD64, ISP_RD32((void *)(ISP_ADDR + 0xD64)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD68, ISP_RD32((void *)(ISP_ADDR + 0xD68)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD6C, ISP_RD32((void *)(ISP_ADDR + 0xD6c)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD70, ISP_RD32((void *)(ISP_ADDR + 0xD70)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD74, ISP_RD32((void *)(ISP_ADDR + 0xD74)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD78, ISP_RD32((void *)(ISP_ADDR + 0xD78)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xD7C, ISP_RD32((void *)(ISP_ADDR + 0xD7C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xDA4, ISP_RD32((void *)(ISP_ADDR + 0xDA4)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xDA8, ISP_RD32((void *)(ISP_ADDR + 0xDA8)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0xDAC, ISP_RD32((void *)(ISP_ADDR + 0xDAC)));
#if 0
	ISP_WR32((void *)(ISP_ADDR + 0x4618), 0x0000FFFF);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4010, ISP_RD32((void *)(ISP_ADDR + 0x4010)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4014, ISP_RD32((void *)(ISP_ADDR + 0x4014)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4018, ISP_RD32((void *)(ISP_ADDR + 0x4018)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x401C, ISP_RD32((void *)(ISP_ADDR + 0x401C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4020, ISP_RD32((void *)(ISP_ADDR + 0x4020)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4024, ISP_RD32((void *)(ISP_ADDR + 0x4024)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4028, ISP_RD32((void *)(ISP_ADDR + 0x4028)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x402C, ISP_RD32((void *)(ISP_ADDR + 0x402C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4100, ISP_RD32((void *)(ISP_ADDR + 0x4100)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4104, ISP_RD32((void *)(ISP_ADDR + 0x4104)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4108, ISP_RD32((void *)(ISP_ADDR + 0x4108)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x410C, ISP_RD32((void *)(ISP_ADDR + 0x410C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4120, ISP_RD32((void *)(ISP_ADDR + 0x4120)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x412C, ISP_RD32((void *)(ISP_ADDR + 0x412C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x420C, ISP_RD32((void *)(ISP_ADDR + 0x420C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4234, ISP_RD32((void *)(ISP_ADDR + 0x4234)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4240, ISP_RD32((void *)(ISP_ADDR + 0x4240)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4300, ISP_RD32((void *)(ISP_ADDR + 0x4300)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4304, ISP_RD32((void *)(ISP_ADDR + 0x4304)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4600, ISP_RD32((void *)(ISP_ADDR + 0x4600)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4604, ISP_RD32((void *)(ISP_ADDR + 0x4604)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4608, ISP_RD32((void *)(ISP_ADDR + 0x4608)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x460C, ISP_RD32((void *)(ISP_ADDR + 0x460C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4610, ISP_RD32((void *)(ISP_ADDR + 0x4610)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4614, ISP_RD32((void *)(ISP_ADDR + 0x4614)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4618, ISP_RD32((void *)(ISP_ADDR + 0x4618)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x10);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x11);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x12);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x10);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x11);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x12);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x10);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x11);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));
	ISP_WR32((void *)(ISP_ADDR + 0x461C), 0x12);
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x461C, ISP_RD32((void *)(ISP_ADDR + 0x461C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4620, ISP_RD32((void *)(ISP_ADDR + 0x4620)));

	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4618, ISP_RD32((void *)(ISP_ADDR + 0x4618)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4624, ISP_RD32((void *)(ISP_ADDR + 0x4624)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4628, ISP_RD32((void *)(ISP_ADDR + 0x4628)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x462C, ISP_RD32((void *)(ISP_ADDR + 0x462C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x4630, ISP_RD32((void *)(ISP_ADDR + 0x4630)));
#endif

	/* Mipi source */
	LOG_DBG("[0x%08X %08X],[0x%08X %08X]", (unsigned int)(0x10215830),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0x830), (unsigned int)(0x10215830),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0x830));
	LOG_DBG("[0x%08X %08X],[0x%08X %08X]", (unsigned int)(0x10215c30),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0xc30), (unsigned int)(0x10215c30),
		(unsigned int)ISP_RD32(ISP_MIPI_ANA_ADDR + 0xc30));

	/* NSCI2 1 debug */
	ISP_WR32((ISP_ADDR + 0x43B8), 0x02);
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x43B8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43B8));
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x43BC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43BC));
	ISP_WR32((ISP_ADDR + 0x43B8), 0x12);
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x43B8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43B8));
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x43BC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43BC));
	/* NSCI2 3 debug */
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x02);
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x4BB8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x43BC));
	ISP_WR32((ISP_ADDR + 0x4BB8), 0x12);
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x43B8),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BB8));
	LOG_DBG("[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x4BBC),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4BBC));

	/* seninf1 */
	LOG_ERR("[0x%08X %08X],[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x4008),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4008), (unsigned int)(ISP_TPIPE_ADDR + 0x4100),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x4100));
	RegDump(0x4120, 0x4160);
	RegDump(0x4360, 0x43f0)
	    /* seninf2 */
	    LOG_ERR("[0x%08X %08X],[0x%08X %08X]", (unsigned int)(ISP_TPIPE_ADDR + 0x4008),
		    (unsigned int)ISP_RD32(ISP_ADDR + 0x4008),
		    (unsigned int)(ISP_TPIPE_ADDR + 0x4100),
		    (unsigned int)ISP_RD32(ISP_ADDR + 0x4100));
	RegDump(0x4520, 0x4560);
	RegDump(0x4600, 0x4610);
	RegDump(0x4760, 0x47f0);


	LOG_DBG("end MT");
	/* for tpipe main end */

	/* INT STX */
	for (i = 0x44; i <= 0x48; i += 4) {
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}

	/* DMA */
	for (i = 0x248; i <= 0x24C; i += 4) {	/* IMGI */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x280; i <= 0x284; i += 4) {	/* LSCI */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x29C; i <= 0x2A0; i += 4) {	/* FLKI */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x2D8; i <= 0x2DC; i += 4) {	/* VIPI */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x2F8; i <= 0x2FC; i += 4) {	/* VIP2I */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x300; i <= 0x31C; i += 4) {	/* IMGO */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x334; i <= 0x338; i += 4) {	/* IMG2O */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x380; i <= 0x384; i += 4) {	/* ESFKO */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}
	for (i = 0x39C; i <= 0x3A0; i += 4) {	/* ESFKO */
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}

	/* DMA ERR ST */
	for (i = 0x3A4; i <= 0x3D8; i += 4) {
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}

	/* TG1 */
	for (i = 0x410; i <= 0x44C; i += 4) {
		LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + i), ISP_RD32(ISP_ADDR + i));
	}

	/* CDP */
	LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + 0xCC0),
		ISP_RD32((void *)(ISP_ADDR + 0xCC0)));
	LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + 0xD40),
		ISP_RD32((void *)(ISP_ADDR + 0xD40)));

	LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_ADDR_CAMINF), ISP_RD32((void *)ISP_ADDR_CAMINF));
	LOG_DBG("0x%08X %08X ", (unsigned int)(ISP_TPIPE_ADDR + 0x150),
		ISP_RD32((void *)(ISP_ADDR + 0x150)));

#if 0
	/*
	   1.    Debug port information:

	   [30]:ultra-high is enable
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x94);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

	/*
	   IMGI :
	   0x15004160 ] 0x00000011
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000011);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   IMGCI :
	   0x15004160 ] 0x00000021
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000021);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   LSCI :
	   0x15004160 ] 0x00000031
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000031);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

	/*
	   FLKI :
	   0x15004160 ] 0x00000051
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000051);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

	/*
	   LCEI :
	   0x15004160 ] 0x00000061
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000061);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   VIPI :
	   0x15004160 ] 0x00000071
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000071);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   VIP2I :
	   0x15004160 ] 0x00000081
	   Read 0x15004164
	   Check bit 21 ->N dmadone_flag
	   Check bit [9:7]  -> NFSM (idle =0)
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000081);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   IMGO
	   0x15004160 ] 0x00000194
	   Read 0x15004164
	   Check bit [4:2] ->NFSM (idle =0)
	   Check bit 23  -> Ndmasent_flag
	   Check bit 22 -> N dmadone_flag
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x00000194);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   IMG2O
	   0x15004160 ] 0x000001a4
	   Read 0x15004164
	   Check bit [4:2] ->NFSM (idle =0)
	   Check bit 23  -> Ndmasent_flag
	   Check bit 22 -> N dmadone_flag
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x000001a4);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

	/*
	   LCSO
	   0x15004160 ] 0x000001b4
	   Read 0x15004164
	   Check bit [4:2] ->NFSM (idle =0)
	   Check bit 23  -> Ndmasent_flag
	   Check bit 22 -> N dmadone_flag
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x000001b4);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

	/*
	   ESFKO
	   0x15004160 ] 0x000001c4
	   Read 0x15004164
	   Check bit [4:2] ->NFSM (idle =0)
	   Check bit 23  -> Ndmasent_flag
	   Check bit 22 -> N dmadone_flag
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x000001c4);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));
	/*
	   AAO
	   0x15004160 ] 0x000001d4
	   Read 0x15004164
	   Check bit [4:2] ->NFSM (idle =0)
	   Check bit 23  -> Ndmasent_flag
	   Check bit 22 -> N dmadone_flag
	 */
	ISP_WR32(ISP_ADDR + 0x160, 0x000001d4);
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x160, ISP_RD32(ISP_ADDR + 0x160));
	LOG_DBG("0x%08X %08X ", ISP_ADDR + 0x164, ISP_RD32(ISP_ADDR + 0x164));

#endif


	/* spin_unlock_irqrestore(&(g_IspInfo.SpinLock), flags); */

	LOG_DBG("-");
	return Ret;
}

/*********************************************************************
1.
0x4160 = 0x9000, look 0x4164 , it is imgo status
0x4160 = 0x9001, look 0x4164 , it is imgo line / pix cnt with sync
0x4160 = 0x9002, look 0x4164 , it is imgo line / pix cnt without sync

0x4160 = 0x9003, look 0x4164 , it is imgi status
0x4160 = 0x9004, look 0x4164, it is imgi line / pix cnt with sync
0x4160 = 0x9005, look 0x4164, it is imgi line / pix cnt without sync

0x4160 = 0x9006, look 0x4164, it is raw_cfa status
0x4160 = 0x9007, look 0x4164, it is raw_cfa line / pix cnt with sync
0x4160 = 0x9008, look 0x4164, it is raw_cfa line / pix cnt without sync

0x4160 = 0x9009, look 0x4164, it is rgb_yuv status
0x4160 = 0x900a, look 0x4164, it is rgb_yuv a line / pix cnt with sync
0x4160 = 0x900b, look 0x4164, it is rgb_yuv line / pix cnt without sync

0x4160 = 0x900c, look 0x4164, it is yuv_out status
0x4160 = 0x900d, look 0x4164, it is yuv_out line / pix cnt with sync
0x4160 = 0x900e, look 0x4164, it is yuv_out line / pix cnt without sync

Status :
Bit 31:28 ? {sot_reg, eol_reg, eot_reg, sof} , reg means status record
Bit 27:24 ?{eot, eol,eot, req}
Bit 23 : rdy

Rdy should be 1    at idle or end of tile, if not 0, 很可能是mdp 沒回rdy
Req  should be 0   at idle or end of tile

sot_reg, eol_reg, eot_reg should be 1  at idle or end of tile
you can also line / pix cnt without sync , to check if

line count  : bit 31:16
pix count  :  bit 15:0


2. 0x4044 / 0x4048 status
      It is 無須 enable,
It is clear by 0x4020[31] write or read clear,
It has many information on it,
You can look coda

3. read CQ status status
     0x4160 = 0x6000
     Dump 0x4164 full register
     Bit 3:0 : cq1 , 1 means idle
     Bit 7:4 : cq2 , 1 means idle
    Bit 11:8 : cq3 , 1 means idle
   Bit 13:12 : apb status , 1 means idle


#define ISP_REG_ADDR_CTL_DBG_SET_IMGI_STS                  (0x9003)
#define ISP_REG_ADDR_CTL_DBG_SET_IMGI_SYNC                (0x9004)
#define ISP_REG_ADDR_CTL_DBG_SET_IMGI_NO_SYNC          (0x9005)

#define ISP_REG_ADDR_CTL_DBG_SET_CFA_STS                  (0x9006)
#define ISP_REG_ADDR_CTL_DBG_SET_CFA_SYNC                (0x9007)
#define ISP_REG_ADDR_CTL_DBG_SET_CFA_NO_SYNC          (0x9008)

#define ISP_REG_ADDR_CTL_DBG_SET_YUV_STS                  (0x9009)
#define ISP_REG_ADDR_CTL_DBG_SET_YUV_SYNC                (0x900a)
#define ISP_REG_ADDR_CTL_DBG_SET_YUV_NO_SYNC          (0x900b)

#define ISP_REG_ADDR_CTL_DBG_SET_OUT_STS                  (0x900c)
#define ISP_REG_ADDR_CTL_DBG_SET_OUT_SYNC                (0x900d)
#define ISP_REG_ADDR_CTL_DBG_SET_OUT_NO_SYNC          (0x900e)
*********************************************************************/
static int ISPDbgPortDump(int params)
{
	MINT32 Ret = 0;

	LOG_DBG("ISP DBG PORT DUMP >>");

	LOG_DBG("CQ STATUS:>");
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG("IMGI STATUS:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_IMGI_STS);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_IMGI_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_IMGI_NO_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("RAW_CFA STATUS:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_CFA_STS);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_CFA_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_CFA_NO_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("RGB_YUV STATUS:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_YUV_STS);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_YUV_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_YUV_NO_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("YUV_OUT STATUS:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_OUT_STS);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_OUT_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_OUT_NO_SYNC);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");


	LOG_DBG("CQ STATUS RETURN:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_CQ_STS);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("INT_STATUSX:>");
	LOG_DBG("(0x%08x 0x%08x)", (unsigned int)(ISP_REG_ADDR_INT_STATUSX),
		ISP_RD32((void *)ISP_REG_ADDR_INT_STATUSX));
	LOG_DBG(" <");
	LOG_DBG("DMA_INTX:>");
	LOG_DBG("(0x%08x 0x%08x)", (unsigned int)(ISP_REG_ADDR_DMA_INTX),
		ISP_RD32((void *)ISP_REG_ADDR_DMA_INTX));
	LOG_DBG(" <");

	LOG_DBG("TDR0:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x5000);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("TDR1:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x5100);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("MDPINF:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0xA000);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("CRSP:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x910F);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG(" <");

	LOG_DBG("PASS2_DB_EN (~0x00000020):>");
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x018, ISP_RD32((void *)(ISP_ADDR + 0x018)));
	LOG_DBG(" <");

	LOG_DBG("TDR_EN (0x80000000):>");
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x054, ISP_RD32((void *)(ISP_ADDR + 0x054)));
	LOG_DBG(" <");


	LOG_DBG("TDR_BASE_ADDR :>");
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x204, ISP_RD32((void *)(ISP_ADDR + 0x204)));
	LOG_DBG(" <");

	LOG_DBG("IMGI :>");
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x230, ISP_RD32((void *)(ISP_ADDR + 0x230)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x234, ISP_RD32((void *)(ISP_ADDR + 0x234)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x238, ISP_RD32((void *)(ISP_ADDR + 0x238)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x23C, ISP_RD32((void *)(ISP_ADDR + 0x23C)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x240, ISP_RD32((void *)(ISP_ADDR + 0x240)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x248, ISP_RD32((void *)(ISP_ADDR + 0x248)));
	LOG_DBG("0x%08X %08X", ISP_TPIPE_ADDR + 0x24C, ISP_RD32((void *)(ISP_ADDR + 0x24C)));
	LOG_DBG(" <");
	LOG_DBG("<CDP_OUT STATUS:>");
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x0000400d);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x0000400e);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x0000400f);
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
	LOG_DBG("ISP DBG PORT DUMP <<");
	return Ret;
}


static void SeninfOverrunDump(void)
{
	static MINT32 debugFlag;

	LOG_DBG("+");
	LOG_DBG("(0xF0203108 0x%08x)", ISP_RD32((void *)0xF0203108));


	if (debugFlag == 0) {

		MUINT32 idx = 0;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1100);
		g_seninfDebug[idx].regVal_1 = 0x1100;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1002);
		g_seninfDebug[idx].regVal_1 = 0x1002;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1102);
		g_seninfDebug[idx].regVal_1 = 0x1102;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1202);
		g_seninfDebug[idx].regVal_1 = 0x1202;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1302);
		g_seninfDebug[idx].regVal_1 = 0x1302;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1006);
		g_seninfDebug[idx].regVal_1 = 0x1006;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1106);
		g_seninfDebug[idx].regVal_1 = 0x1106;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x110A);
		g_seninfDebug[idx].regVal_1 = 0x110A;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x100B);
		g_seninfDebug[idx].regVal_1 = 0x100B;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x110B);
		g_seninfDebug[idx].regVal_1 = 0x110B;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x110C);
		g_seninfDebug[idx].regVal_1 = 0x110C;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1016);
		g_seninfDebug[idx].regVal_1 = 0x1016;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1017);
		g_seninfDebug[idx].regVal_1 = 0x1017;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1018);
		g_seninfDebug[idx].regVal_1 = 0x1018;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x1019);
		g_seninfDebug[idx].regVal_1 = 0x1019;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x101A);
		g_seninfDebug[idx].regVal_1 = 0x101A;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x101B);
		g_seninfDebug[idx].regVal_1 = 0x101B;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x9000);
		g_seninfDebug[idx].regVal_1 = 0x9000;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x9001);
		g_seninfDebug[idx].regVal_1 = 0x9001;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, 0x9002);
		g_seninfDebug[idx].regVal_1 = 0x9002;
		g_seninfDebug[idx].regVal_2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT);
		idx++;

		debugFlag = 1;
	}

	LOG_DBG("-");
}


int ISPRegDump(int params, int params1)
{
	MINT32 Ret = 0;

	LOG_DBG("+");

	/* spin_lock_irqsave(&(g_IspInfo.SpinLock), flags); */

	/* tile tool parse range */
	/* Joseph Hung (xa)#define ISP_ADDR_START  0x15004000 */
	/* #define ISP_ADDR_END    0x15006000 */
	/*  */

	/* for tpipe main start */
	LOG_DBG("Camera_isp RegDump start");
	LOG_DBG("start MT");
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x000,
		ISP_RD32((void *)(ISP_ADDR + 0x000)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x004,
		ISP_RD32((void *)(ISP_ADDR + 0x004)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x008,
		ISP_RD32((void *)(ISP_ADDR + 0x008)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x00C,
		ISP_RD32((void *)(ISP_ADDR + 0x00C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x010,
		ISP_RD32((void *)(ISP_ADDR + 0x010)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x018,
		ISP_RD32((void *)(ISP_ADDR + 0x018)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x01C,
		ISP_RD32((void *)(ISP_ADDR + 0x01C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x050,
		ISP_RD32((void *)(ISP_ADDR + 0x050)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x054,
		ISP_RD32((void *)(ISP_ADDR + 0x054)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x078,
		ISP_RD32((void *)(ISP_ADDR + 0x078)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x110,
		ISP_RD32((void *)(ISP_ADDR + 0x110)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x114,
		ISP_RD32((void *)(ISP_ADDR + 0x114)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x22C,
		ISP_RD32((void *)(ISP_ADDR + 0x22C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x240,
		ISP_RD32((void *)(ISP_ADDR + 0x240)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x27C,
		ISP_RD32((void *)(ISP_ADDR + 0x27C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x2B4,
		ISP_RD32((void *)(ISP_ADDR + 0x2B4)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x308,
		ISP_RD32((void *)(ISP_ADDR + 0x308)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x30C,
		ISP_RD32((void *)(ISP_ADDR + 0x30C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x310,
		ISP_RD32((void *)(ISP_ADDR + 0x310)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x31C,
		ISP_RD32((void *)(ISP_ADDR + 0x31C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x328,
		ISP_RD32((void *)(ISP_ADDR + 0x328)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x32C,
		ISP_RD32((void *)(ISP_ADDR + 0x32C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x330,
		ISP_RD32((void *)(ISP_ADDR + 0x330)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x33C,
		ISP_RD32((void *)(ISP_ADDR + 0x33C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x3A8,
		ISP_RD32((void *)(ISP_ADDR + 0x3A8)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x534,
		ISP_RD32((void *)(ISP_ADDR + 0x534)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x538,
		ISP_RD32((void *)(ISP_ADDR + 0x538)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x53C,
		ISP_RD32((void *)(ISP_ADDR + 0x53C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x800,
		ISP_RD32((void *)(ISP_ADDR + 0x800)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x8A0,
		ISP_RD32((void *)(ISP_ADDR + 0x8A0)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x9C4,
		ISP_RD32((void *)(ISP_ADDR + 0x9C4)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x9E4,
		ISP_RD32((void *)(ISP_ADDR + 0x9E4)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x9E8,
		ISP_RD32((void *)(ISP_ADDR + 0x9E8)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0x9EC,
		ISP_RD32((void *)(ISP_ADDR + 0x9EC)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xA20,
		ISP_RD32((void *)(ISP_ADDR + 0xA20)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xACC,
		ISP_RD32((void *)(ISP_ADDR + 0xACC)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB00,
		ISP_RD32((void *)(ISP_ADDR + 0xB00)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB04,
		ISP_RD32((void *)(ISP_ADDR + 0xB04)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB08,
		ISP_RD32((void *)(ISP_ADDR + 0xB08)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB0C,
		ISP_RD32((void *)(ISP_ADDR + 0xB0C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB10,
		ISP_RD32((void *)(ISP_ADDR + 0xB10)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB14,
		ISP_RD32((void *)(ISP_ADDR + 0xB14)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB18,
		ISP_RD32((void *)(ISP_ADDR + 0xB18)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB1C,
		ISP_RD32((void *)(ISP_ADDR + 0xB1C)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xB20,
		ISP_RD32((void *)(ISP_ADDR + 0xB20)));
	LOG_DBG("[ISP/MDP][TPIPE_DumpReg] 0x%08X = 0x%08X", ISP_TPIPE_ADDR + 0xF50,
		ISP_RD32((void *)(ISP_ADDR + 0xF50)));
	LOG_DBG("end MT");


	/* spin_unlock_irqrestore(&(g_IspInfo.SpinLock), flags); */

	LOG_DBG("-");
	ISPDbgPortDump(params);
	return Ret;
}

static int ISPResetPass2(int params)
{
	MINT32 i = 0;
	MINT32 Ret = 0;
	MINT32 m32RegEn2 = 0;
	MINT32 m32RegCropX = 0;

	LOG_DBG("ISPResetPass2 >>");


/* LOG_DBG("CQ STATUS: bef"); */
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));


	LOG_DBG("ISP_REG_ADDR_CTL_EN2>: 0x%08x ", ISP_RD32((void *)ISP_REG_ADDR_CTL_EN2));
	m32RegEn2 = ISP_RD32((void *)ISP_REG_ADDR_CTL_EN2);
	ISP_WR32((void *)ISP_REG_ADDR_CTL_EN2, m32RegEn2 & (~ISP_REG_ADDR_CTL_EN2_UV_CRSA_EN_BIT));
/* LOG_DBG("ISP_REG_ADDR_CTL_EN2>>: 0x%08x ", ISP_RD32((void *)ISP_REG_ADDR_CTL_EN2)); */

	LOG_DBG("ISP_REG_ADDR_CTL_CROP_X>: 0x%08x ", ISP_RD32((void *)ISP_REG_ADDR_CTL_CROP_X));
	m32RegCropX = ISP_RD32((void *)ISP_REG_ADDR_CTL_CROP_X);
	ISP_WR32((void *)ISP_REG_ADDR_CTL_CROP_X,
		 m32RegCropX & (~ISP_REG_ADDR_CTL_CROP_X_MDP_CROP_EN_BIT));
/* LOG_DBG("ISP_REG_ADDR_CTL_CROP_X>>: 0x%08x ", ISP_RD32((void *)ISP_REG_ADDR_CTL_CROP_X)); */

	ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_CQ_STS);
	for (i = 0; i < 5000; i++) {
		if ((ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT) & 0x11) == 0x11) {
			break;
		}
	}
	LOG_DBG("i - %d ", i);
	ISP_WR32((void *)ISP_REG_ADDR_CTL_EN2, m32RegEn2);
/* LOG_DBG("ISP_REG_ADDR_CTL_EN2>>>: 0x%08x ", ISP_RD32((void *)ISP_REG_ADDR_CTL_EN2)); */

	ISP_WR32((void *)ISP_REG_ADDR_CTL_CROP_X, m32RegCropX);
/* LOG_DBG("ISP_REG_ADDR_CTL_CROP_X>>>: 0x%08x ", ISP_RD32((void *)ISP_REG_ADDR_CTL_CROP_X)); */

/* LOG_DBG("CQ STATUS: aft"); */
	LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
		ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));


	LOG_DBG("ISPResetPass2 <<");
	return Ret;
}

int MDPReset_Process(int params)
{
	LOG_DBG("\n\n\n MDP cmdqReset_cb Test %d\n\n\n", params);
	ISPResetPass2(params);
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MVOID ISP_EnableClock(MBOOL En, MUINT32 LogLevel)
{

	if (LogLevel > 0) {
		LOG_INF("En(%d),g_EnableClkCnt(%d)", En, g_EnableClkCnt);
	}

	if (En) {		/* enable clock. */
		enable_clock(MT_CG_DISP0_SMI_COMMON, "CAMERA");
		enable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
		enable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
		enable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
		enable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
		enable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
		g_EnableClkCnt++;
		/*LOG_DBG("Camera clock enbled. g_EnableClkCnt(%d)", g_EnableClkCnt);*/

		/*LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
			ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));*/
		if (ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET) != ISP_REG_ADDR_CTL_DBG_SET_CQ_STS) {
/* LOG_DBG("CQ STATUS RETURN:>"); */
			ISP_WR32((void *)ISP_REG_ADDR_CTL_DBG_SET, ISP_REG_ADDR_CTL_DBG_SET_CQ_STS);
			if (LogLevel > 0) {
				LOG_DBG("(0x%08x 0x%08x)", ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_SET),
					ISP_RD32((void *)ISP_REG_ADDR_CTL_DBG_PORT));
			}
/* LOG_DBG(" <"); */
		}
	} else {		/* disable clock. */

		disable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
		disable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
		disable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
		disable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
		disable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
		disable_clock(MT_CG_DISP0_SMI_COMMON, "CAMERA");
		g_EnableClkCnt--;
		/*LOG_DBG("Camera clock disabled. g_EnableClkCnt(%d)", g_EnableClkCnt);*/
	}

	if (LogLevel > 0) {
		LOG_DBG("-");
	}
}

/*******************************************************************************
*
********************************************************************************/
static inline MVOID ISP_Reset(MVOID)
{
	/* ensure the view finder is disabe. 0: take_picture */
	/* ISP_CLR_BIT(ISP_REG_ADDR_EN1, 0); */

	MUINT32 i, Reg;
	unsigned long flags;

	/*LOG_DBG("+");*/

	/* TODO: MUST remove later */
	/* imgsys clk on */
	/* ISP_WR32(ISP_ADDR_CAMINF, 0); */
	/* ISP_EnableClock(MTRUE); */
	LOG_DBG("+ isp gate clk(0x%x)", ISP_RD32((void *)ISP_ADDR_CAMINF));

	/* imgsys IMG_SW_RST -> reset HW register */
	/*
	   1500000C    IMG_SW_RST
	   17    16    SENINF    SENINF    R/W    2'b00    "SENINF clock domain hw reset
	   bit 0 : cam_tg hw domain hw reset,
	   bit 1 : (TBD)
	   0 : no hw reset
	   1 : hw reset"

	   14    12    CAM_RST    CAM_RST    R/W    3'b000    " CAM W reset
	   bit 0 : fsmi domain HW reset
	   bit 1 : fmem domain HW reset
	   bit 2 : faxi(cpu bus) dom HW reset
	   0 : no hw reset
	   1 : hw reset"
	 */
/* Reg = ISP_RD32(ISP_ADDR_CAMINF+0x0C); */
/* ISP_WR32(ISP_ADDR_CAMINF+0x0C, (Reg|0x0003F000)); */
/* mdelay(5); */
/* ISP_WR32(ISP_ADDR_CAMINF+0x0C, (Reg&(~0x0003F000)) ); */

/* js_test, remove later once IMGSYS is working. Now it's NOT working on FPGA. */
/* remove temp solutionfor(i = 0; i <= 0x54; i += 4) */
/* { */
/* remove temp solution    ISP_WR32((void *)(ISP_ADDR + i), 0x00 ); */
/* } */

	LOG_DBG("remove 0x0~0x54 = 0x0");	/* remove temp solution */

	/* bandwidth limitor for TG */
#if 0
	Reg = ISP_RD32((void *)(EMI_BASE + 0x120));
	Reg |= 0x3F;

	ISP_WR32((void *)(EMI_BASE + 0x120), Reg);
	ISP_WR32((void *)ISP_REG_ADDR_SW_CTL, ISP_REG_SW_CTL_SW_RST_TRIG);
#endif
	/*
	   while(1)
	   {
	   Reg = ISP_RD32(ISP_REG_ADDR_SW_CTL);
	   if(Reg & ISP_REG_SW_CTL_SW_RST_STATUS)
	   {
	   break;
	   }
	   }
	 */

	do {
		Reg = ISP_RD32((void *)ISP_REG_ADDR_SW_CTL);
	} while (0/* (!Reg) & ISP_REG_SW_CTL_SW_RST_STATUS */);

	ISP_WR32((void *)ISP_REG_ADDR_SW_CTL, ISP_REG_SW_CTL_SW_RST_TRIG | ISP_REG_SW_CTL_HW_RST);	/* 0x5 */
	ISP_WR32((void *)ISP_REG_ADDR_SW_CTL, ISP_REG_SW_CTL_HW_RST);	/* 0x4 */
	ISP_WR32((void *)ISP_REG_ADDR_SW_CTL, 0);

	spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);

	for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
		g_IspInfo.IrqInfo.Status[i] = 0;
	}

	spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);

	g_TempAddr = 0;

	LOG_DBG("-");
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_ReadReg(ISP_REG_IO_STRUCT *pRegIo)
{
	/* MUINT32 *pData = (MUINT32 *)pRegIo->Data; */
	ISP_REG_STRUCT *pData = pRegIo->pData;
	MUINT32 i;
	MINT32 Ret = 0;

	ISP_REG_STRUCT reg;

	/* ====== Read Register ====== */

	for (i = 0; i < pRegIo->Count; i++) {
		if (0 != get_user(reg.Addr, &(pData->Addr))) {
			LOG_ERR("get_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		if ((ISP_ADDR_CAMINF + reg.Addr >= ISP_ADDR)
		    && (ISP_ADDR_CAMINF + reg.Addr < (ISP_ADDR_CAMINF + ISP_RANGE))) {
			reg.Val = ISP_RD32((ISP_ADDR_CAMINF + reg.Addr));
		} else {
			LOG_ERR("Wrong address(0x%x)", (unsigned int)(ISP_ADDR_CAMINF + reg.Addr));
			reg.Val = 0;
		}
		if (0 != put_user(reg.Val, &(pData->Val))) {
			LOG_ERR("put_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		pData++;
	}

EXIT:
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_WriteRegToHw(ISP_REG_STRUCT *pReg, MUINT32 Count)
{
	MINT32 Ret = 0;
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_WRITE_REG) {
		LOG_DBG("+");
	}

	spin_lock(&(g_IspInfo.SpinLockIsp));
	for (i = 0; i < Count; i++) {
		if (g_IspInfo.DebugMask & ISP_DBG_WRITE_REG) {
			LOG_DBG("Addr(0x%08X), Val(0x%08X)",
				(MUINT32) (ISP_ADDR_CAMINF + pReg[i].Addr),
				(MUINT32) (pReg[i].Val));
		}
		if (((ISP_ADDR_CAMINF + pReg[i].Addr) >= ISP_ADDR)
		    && ((ISP_ADDR_CAMINF + pReg[i].Addr) < (ISP_ADDR_CAMINF + ISP_RANGE))) {
			ISP_WR32((ISP_ADDR_CAMINF + pReg[i].Addr), pReg[i].Val);
		} else {
			LOG_ERR("wrong address(0x%x)",
				(unsigned int)(ISP_ADDR_CAMINF + pReg[i].Addr));
		}
	}
	spin_unlock(&(g_IspInfo.SpinLockIsp));

	return Ret;
}

/*******************************************************************************
*
********************************************************************************
static void ISP_BufWrite_Init(void)    //Vent@20121106: Marked to remove build warning: 'ISP_BufWrite_Init' defined but not used [-Wunused-function]
{
    MUINT32 i;
    //
    if(g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE)
    {
	LOG_DBG("- E.");
    }
    //
    for(i=0; i<ISP_BUF_WRITE_AMOUNT; i++) {
	g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
	g_IspInfo.BufInfo.Write[i].Size = 0;
	g_IspInfo.BufInfo.Write[i].pData = NULL;
    }
}

*******************************************************************************
*
********************************************************************************/
static MVOID ISP_BufWrite_Dump(MVOID)
{
	MUINT32 i;

	LOG_DBG("+");

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		LOG_DBG("i(%d),Status(%d),Size(%d)", i, g_IspInfo.BufInfo.Write[i].Status,
			g_IspInfo.BufInfo.Write[i].Size);
		g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
		g_IspInfo.BufInfo.Write[i].Size = 0;
		g_IspInfo.BufInfo.Write[i].pData = NULL;
	}
}

/*******************************************************************************
*
********************************************************************************/
static MVOID ISP_BufWrite_Free(MVOID)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
		g_IspInfo.BufInfo.Write[i].Size = 0;

		if (g_IspInfo.BufInfo.Write[i].pData != NULL) {
			kfree(g_IspInfo.BufInfo.Write[i].pData);
			g_IspInfo.BufInfo.Write[i].pData = NULL;
		}
	}
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Alloc(MVOID)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
		g_IspInfo.BufInfo.Write[i].Size = 0;
		g_IspInfo.BufInfo.Write[i].pData =
		    (MUINT8 *) kmalloc(ISP_BUF_SIZE_WRITE, GFP_ATOMIC);
		if (g_IspInfo.BufInfo.Write[i].pData == NULL) {
			LOG_DBG("ERROR: i = %d, pData is NULL", i);
			ISP_BufWrite_Free();
			return MFALSE;
		}
	}

	return MTRUE;
}

/*******************************************************************************
*
********************************************************************************/
static MVOID ISP_BufWrite_Reset(MVOID)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_EMPTY;
		g_IspInfo.BufInfo.Write[i].Size = 0;
	}
}

/*******************************************************************************
*
********************************************************************************/
static inline MUINT32 ISP_BufWrite_GetAmount(MVOID)
{
	MUINT32 i;
	MUINT32 Count = 0;

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		if (g_IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_READY) {
			Count++;
		}
	}

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("Count(%d)", Count);
	}

	return Count;
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Add(MUINT32 Size, ISP_REG_STRUCT *pData)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	/* write to hold buffer */
	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		if (g_IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_HOLD) {
			if ((g_IspInfo.BufInfo.Write[i].Size + Size) > ISP_BUF_SIZE_WRITE) {
				LOG_ERR("i(%d), BufWriteSize(%d) + Size(%d) > %d", i,
					g_IspInfo.BufInfo.Write[i].Size, Size, ISP_BUF_SIZE_WRITE);
				return MFALSE;
			}

			if (copy_from_user
			    ((void *)(g_IspInfo.BufInfo.Write[i].pData +
				      g_IspInfo.BufInfo.Write[i].Size), (void *)pData, Size) != 0) {
				LOG_ERR("copy_from_user failed");
				return MFALSE;
			}

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
				LOG_DBG("i(%d), BufSize(%d), Size(%d)", i,
					g_IspInfo.BufInfo.Write[i].Size, Size);
			}

			g_IspInfo.BufInfo.Write[i].Size += Size;
			return MTRUE;
		}
	}

	/* write to empty buffer */
	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		if (g_IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_EMPTY) {
			if (Size > ISP_BUF_SIZE_WRITE) {
				LOG_ERR("i(%d), Size(%d) > %d", i, Size, ISP_BUF_SIZE_WRITE);
				return MFALSE;
			}

			if (copy_from_user
			    ((void *)(g_IspInfo.BufInfo.Write[i].pData), (void __user *)pData,
			     Size) != 0) {
				LOG_ERR("copy_from_user failed");
				return MFALSE;
			}

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
				LOG_DBG("i = %d, Size = %d", i, Size);
			}

			g_IspInfo.BufInfo.Write[i].Size = Size;
			g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_HOLD;

			return MTRUE;
		}
	}

	LOG_ERR("All write buffer are full of data!");
	return MFALSE;
}

/*******************************************************************************
*
********************************************************************************/
static MVOID ISP_BufWrite_SetReady(MVOID)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		if (g_IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_HOLD) {
			if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
				LOG_DBG("i(%d), Size(%d)", i, g_IspInfo.BufInfo.Write[i].Size);
			}
			g_IspInfo.BufInfo.Write[i].Status = ISP_BUF_STATUS_READY;
		}
	}
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Get(MUINT32 * pIndex, MUINT32 *pSize, MUINT8 **ppData)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	for (i = 0; i < ISP_BUF_WRITE_AMOUNT; i++) {
		if (g_IspInfo.BufInfo.Write[i].Status == ISP_BUF_STATUS_READY) {
			if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
				LOG_DBG("i(%d), Size(%d)", i, g_IspInfo.BufInfo.Write[i].Size);
			}

			*pIndex = i;
			*pSize = g_IspInfo.BufInfo.Write[i].Size;
			*ppData = g_IspInfo.BufInfo.Write[i].pData;

			return MTRUE;
		}
	}

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("No buf is ready!");
	}
	return MFALSE;
}

/*******************************************************************************
*
********************************************************************************/
static MBOOL ISP_BufWrite_Clear(MUINT32 Index)
{
	if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
		LOG_DBG("+");
	}

	if (g_IspInfo.BufInfo.Write[Index].Status == ISP_BUF_STATUS_READY) {
		if (g_IspInfo.DebugMask & ISP_DBG_BUF_WRITE) {
			LOG_DBG("Index(%d), Size(%d)", Index, g_IspInfo.BufInfo.Write[Index].Size);
		}
		g_IspInfo.BufInfo.Write[Index].Size = 0;
		g_IspInfo.BufInfo.Write[Index].Status = ISP_BUF_STATUS_EMPTY;

		return MTRUE;
	} else {
		LOG_DBG("WARNING: Index(%d) is not ready! Status(%d)", Index,
			g_IspInfo.BufInfo.Write[Index].Status);
		return MFALSE;
	}
}

/*******************************************************************************
*
********************************************************************************/
static MVOID ISP_BufWrite_WriteToHw(MVOID)
{
	MUINT8 *pBuf;
	MUINT32 Index, BufSize;

	LOG_DBG("+");

	spin_lock(&(g_IspInfo.SpinLockHold));

	while (ISP_BufWrite_Get(&Index, &BufSize, &pBuf)) {
		if (g_IspInfo.DebugMask & ISP_DBG_TASKLET) {
			LOG_DBG("Index(%d), BufSize(%d)", Index, BufSize);
		}

		ISP_WriteRegToHw((ISP_REG_STRUCT *) pBuf, BufSize / sizeof(ISP_REG_STRUCT));
		ISP_BufWrite_Clear(Index);
	}

	/* LOG_DBG("No more buf."); */
	atomic_set(&(g_IspInfo.HoldInfo.WriteEnable), 0);
	wake_up_interruptible(&(g_IspInfo.WaitQueueHead));

	spin_unlock(&(g_IspInfo.SpinLockHold));
}


/*******************************************************************************
*
********************************************************************************/
MVOID ISP_ScheduleWork_VD(struct work_struct *data)
{
	if (g_IspInfo.DebugMask & ISP_DBG_SCHEDULE_WORK) {
		LOG_DBG("+");
	}

	g_IspInfo.TimeLog.WorkQueueVd = ISP_JiffiesToMs(jiffies);

	if (g_IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_VD].Func != NULL) {
		g_IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_VD].Func();
	}
}

/*******************************************************************************
*
********************************************************************************/
MVOID ISP_ScheduleWork_EXPDONE(struct work_struct *data)
{
	if (g_IspInfo.DebugMask & ISP_DBG_SCHEDULE_WORK) {
		LOG_DBG("+");
	}

	g_IspInfo.TimeLog.WorkQueueExpdone = ISP_JiffiesToMs(jiffies);

	if (g_IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_EXPDONE].Func != NULL) {
		g_IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_EXPDONE].Func();
	}
}

/*******************************************************************************
*
********************************************************************************/
MVOID ISP_ScheduleWork_SENINF(struct work_struct *data)
{
	if (g_IspInfo.DebugMask & ISP_DBG_SCHEDULE_WORK) {
		LOG_DBG("+");
	}

	g_IspInfo.TimeLog.WorkQueueSeninf = ISP_JiffiesToMs(jiffies);

	if (g_IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_SENINF].Func != NULL) {
		g_IspInfo.Callback[ISP_CALLBACK_WORKQUEUE_SENINF].Func();
	}
}


/*******************************************************************************
*
********************************************************************************/
MVOID ISP_Tasklet_VD(unsigned long Param)
{
	if (g_IspInfo.DebugMask & ISP_DBG_TASKLET) {
		LOG_DBG("+");
	}

	g_IspInfo.TimeLog.TaskletVd = ISP_JiffiesToMs(jiffies);

	if (g_IspInfo.Callback[ISP_CALLBACK_TASKLET_VD].Func != NULL) {
		g_IspInfo.Callback[ISP_CALLBACK_TASKLET_VD].Func();
	}
	/*  */
	if (g_IspInfo.HoldInfo.Time == ISP_HOLD_TIME_VD) {
		ISP_BufWrite_WriteToHw();
	}
}

DECLARE_TASKLET(IspTaskletVD, ISP_Tasklet_VD, 0);

/*******************************************************************************
*
********************************************************************************/
void ISP_Tasklet_EXPDONE(unsigned long Param)
{
	if (g_IspInfo.DebugMask & ISP_DBG_TASKLET) {
		LOG_DBG("+");
	}

	g_IspInfo.TimeLog.TaskletExpdone = ISP_JiffiesToMs(jiffies);

	if (g_IspInfo.Callback[ISP_CALLBACK_TASKLET_EXPDONE].Func != NULL) {
		g_IspInfo.Callback[ISP_CALLBACK_TASKLET_EXPDONE].Func();
	}

	if (g_IspInfo.HoldInfo.Time == ISP_HOLD_TIME_EXPDONE) {
		ISP_BufWrite_WriteToHw();
	}
}

DECLARE_TASKLET(IspTaskletEXPDONE, ISP_Tasklet_EXPDONE, 0);

/*******************************************************************************
*
********************************************************************************/
void ISP_Tasklet_SENINF(unsigned long Param)
{
	MUINT32 i;

	if (g_IspInfo.DebugMask & ISP_DBG_TASKLET) {
		LOG_DBG("+");
	}

	g_IspInfo.TimeLog.TaskletSeninf = ISP_JiffiesToMs(jiffies);

	/* SeninfOverrunDump(); */

	LOG_DBG("SENINF_OVERRUN\n");
	for (i = 0; i < 20; i++) {
		LOG_DBG("(0x%08x,0x%08x)\n", g_seninfDebug[i].regVal_1, g_seninfDebug[i].regVal_2);
	}
}

DECLARE_TASKLET(IspTaskletSENIF, ISP_Tasklet_SENINF, 0);


/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_WriteReg(ISP_REG_IO_STRUCT *pRegIo)
{
	MINT32 Ret = 0;
	MINT32 TimeVd = 0;
	MINT32 TimeExpdone = 0;
	MINT32 TimeTasklet = 0;
	/* MUINT8 *pData = NULL; */
	ISP_REG_STRUCT *pData = NULL;

	if (g_IspInfo.DebugMask & ISP_DBG_WRITE_REG) {
		/* LOG_DBG("Data(0x%08X), Count(%d)", (MUINT32)(pRegIo->Data), (MUINT32)(pRegIo->Count)); */
		LOG_DBG("Data(0x%p), Count(%d)", pRegIo->pData, pRegIo->Count);
	}

	if (atomic_read(&(g_IspInfo.HoldInfo.HoldEnable))) {
		if (ISP_BufWrite_Add((pRegIo->Count) * sizeof(ISP_REG_STRUCT), pRegIo->pData)) {
			/* LOG_DBG("Add write buffer OK"); */
		} else {
			LOG_ERR("Add write buffer fail");

			TimeVd = ISP_JiffiesToMs(jiffies) - g_IspInfo.TimeLog.Vd;
			TimeExpdone = ISP_JiffiesToMs(jiffies) - g_IspInfo.TimeLog.Expdone;
			TimeTasklet = ISP_JiffiesToMs(jiffies) - g_IspInfo.TimeLog.TaskletExpdone;

			LOG_ERR("HoldTime(%d),VD(%d ms),Expdone(%d ms),Tasklet(%d ms)",
				g_IspInfo.HoldInfo.Time, TimeVd, TimeExpdone, TimeTasklet);
			ISP_BufWrite_Dump();
			ISP_DumpReg();

			Ret = -EFAULT;
			goto EXIT;
		}
	} else {
		/* pData = (MUINT8*)kmalloc((pRegIo->Count)*sizeof(ISP_REG_STRUCT), GFP_ATOMIC); */
		pData =
		    (ISP_REG_STRUCT *) kmalloc((pRegIo->Count) * sizeof(ISP_REG_STRUCT),
					       GFP_ATOMIC);
		if (pData == NULL) {
			LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)",
				current->comm, current->pid, current->tgid);
			Ret = -ENOMEM;
			goto EXIT;
		}
		if (copy_from_user
		    (pData, (void __user *)(pRegIo->pData),
		     pRegIo->Count * sizeof(ISP_REG_STRUCT)) != 0) {
			LOG_ERR("copy_from_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}

		Ret = ISP_WriteRegToHw(pData, pRegIo->Count);
	}

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
static MINT32 ISP_SetHoldTime(ISP_HOLD_TIME_ENUM HoldTime)
{
	LOG_DBG("HoldTime(%d)", HoldTime);
	g_IspInfo.HoldInfo.Time = HoldTime;

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_ResetBuf(MVOID)
{
	LOG_DBG("hold_reg(%d), BufAmount(%d)", atomic_read(&(g_IspInfo.HoldInfo.HoldEnable)),
		ISP_BufWrite_GetAmount());

	ISP_BufWrite_Reset();
	atomic_set(&(g_IspInfo.HoldInfo.HoldEnable), 0);
	atomic_set(&(g_IspInfo.HoldInfo.WriteEnable), 0);

	LOG_DBG("-");
	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_EnableHoldReg(MBOOL En)
{
	MINT32 Ret = 0;
	MUINT32 BufAmount = 0;

	if (g_IspInfo.DebugMask & ISP_DBG_HOLD_REG) {
		LOG_DBG("En(%d), HoldEnable(%d)", En,
			atomic_read(&(g_IspInfo.HoldInfo.HoldEnable)));
	}

	if (!spin_trylock_bh(&(g_IspInfo.SpinLockHold))) {
		/* Should wait until tasklet done. */
		MINT32 Timeout;
		MINT32 IsLock = 0;

		if (g_IspInfo.DebugMask & ISP_DBG_TASKLET) {
			LOG_DBG("Start wait");
		}

		Timeout = wait_event_interruptible_timeout(g_IspInfo.WaitQueueHead,
							   (IsLock =
							    spin_trylock_bh(&
									    (g_IspInfo.
									     SpinLockHold))),
							   ISP_MsToJiffies(500));

		if (g_IspInfo.DebugMask & ISP_DBG_TASKLET) {
			LOG_DBG("End wait ");
		}

		if (IsLock == 0) {
			LOG_ERR("Should not happen, Timeout & IsLock is 0");
			Ret = -EFAULT;
			goto EXIT;
		}
	}

	/* Here we get the lock. */
	if (En == MFALSE) {
		ISP_BufWrite_SetReady();
		BufAmount = ISP_BufWrite_GetAmount();

		if (BufAmount) {
			atomic_set(&(g_IspInfo.HoldInfo.WriteEnable), 1);
		}
	}

	if (g_IspInfo.DebugMask & ISP_DBG_HOLD_REG) {
		LOG_DBG("En(%d), HoldEnable(%d), BufAmount(%d)", En,
			atomic_read(&(g_IspInfo.HoldInfo.HoldEnable)), BufAmount);
	}

	atomic_set(&(g_IspInfo.HoldInfo.HoldEnable), En);

	spin_unlock_bh(&(g_IspInfo.SpinLockHold));

EXIT:
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static long ISP_REF_CNT_CTRL_FUNC(unsigned long Param)
{
	MINT32 Ret = 0;
	ISP_REF_CNT_CTRL_STRUCT ref_cnt_ctrl;
	MINT32 imem_ref_cnt = 0;

	/* add lock here */
	/* spin_lock_irq(&(g_IspInfo.SpinLock)); */

	if (g_IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
		LOG_DBG("+");
	}

	if (NULL == (void __user *)Param) {
		LOG_ERR("NULL Param");
		/* add unlock here */
		/* spin_unlock_irqrestore(&(g_IspInfo.SpinLock), flags); */

		return -EFAULT;
	}

	if (copy_from_user(&ref_cnt_ctrl, (void __user *)Param, sizeof(ISP_REF_CNT_CTRL_STRUCT)) ==
	    0) {
		if (g_IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
			LOG_DBG("ctrl(%d),id(%d)", ref_cnt_ctrl.ctrl, ref_cnt_ctrl.id);
		}

		if (ISP_REF_CNT_ID_MAX > ref_cnt_ctrl.id) {
			/* lock here */
			spin_lock(&(g_IspInfo.SpinLockIspRef));

			switch (ref_cnt_ctrl.ctrl) {
			case ISP_REF_CNT_GET:
				break;
			case ISP_REF_CNT_INC:
				atomic_inc(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
				/* g_imem_ref_cnt++; */
				break;
			case ISP_REF_CNT_DEC:
			case ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE:
				atomic_dec(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
				/* g_imem_ref_cnt--; */
				break;
			default:
			case ISP_REF_CNT_MAX:	/* Add this to remove build warning. */
				/* Do nothing. */
				break;
			}

			imem_ref_cnt = (MINT32) atomic_read(&g_imem_ref_cnt[ref_cnt_ctrl.id]);
			/*LOG_DBG("g_imem_ref_cnt[%d]: %d.", ref_cnt_ctrl.id, imem_ref_cnt);*/

			if ((imem_ref_cnt == 0)) {
				if(ref_cnt_ctrl.ctrl == ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE) /* No user left and ctrl is RESET_IF_LAST_ONE, do ISP reset. */
				{
                    /* camera1. single thread, we rest p1p2 after all the object are destroyed */
                    ISP_Reset();
                    LOG_DBG("ISP_REF_CNT_DEC_AND_RESET_IF_LAST_ONE. Do ISP_Reset");
				}
				/* we do not care ISP_REF_CNT_DEC_AND_RESET_P1_IF_LAST_ONE and ISP_REF_CNT_DEC_AND_RESET_P2_IF_LAST_ONE */
				/* due to we could only rest all in this series */
			}

			/* unlock here */
			spin_unlock(&(g_IspInfo.SpinLockIspRef));

			if (g_IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
				LOG_DBG("ref_cnt(%d)", imem_ref_cnt);
			}

			if (copy_to_user
			    ((void __user *)ref_cnt_ctrl.data_ptr, &imem_ref_cnt,
			     sizeof(MINT32)) != 0) {
				LOG_ERR("[GET]:copy_to_user failed");
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("id(%d) exceed", ref_cnt_ctrl.id);
			Ret = -EFAULT;
		}
	} else {
		LOG_ERR("copy_from_user failed");
		Ret = -EFAULT;
	}

	if (g_IspInfo.DebugMask & ISP_DBG_REF_CNT_CTRL) {
		LOG_DBG("-");
	}

	/* add unlock here */
	/* spin_unlock_irqrestore(&(g_IspInfo.SpinLock), flags); */

	/*LOG_DBG("Ret(%d)", Ret);*/
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_RTBC_ENQUE(MINT32 dma)
{
	MINT32 Ret = 0;
	MINT32 rt_dma = dma;
	MUINT32 buffer_exist = 0;
	MUINT32 i = 0;
	MUINT32 index = 0;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("[rtbc][ENQUE]+");
	}

	/* check max */
	if (ISP_RT_BUF_SIZE == g_pstRTBuf->ring_buf[rt_dma].total_count) {
		LOG_ERR("[rtbc][ENQUE]:real time buffer number FULL:rt_dma(%d)", rt_dma);
		Ret = -EFAULT;
	}

	/* spin_lock_irqsave(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

#if 0
	/* check if buffer exist */
	for (i = 0; i < ISP_RT_BUF_SIZE; i++) {
		if (g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == g_rt_buf_info.base_pAddr) {
			buffer_exist = 1;
			break;
		}

		if (g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == 0) {
			break;
		}
	}
#endif

	if (buffer_exist) {
		if (ISP_RTBC_BUF_EMPTY != g_pstRTBuf->ring_buf[rt_dma].data[i].bFilled) {
			g_pstRTBuf->ring_buf[rt_dma].data[i].bFilled = ISP_RTBC_BUF_EMPTY;
			g_pstRTBuf->ring_buf[rt_dma].empty_count++;
			index = i;
		}

		/* if(g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) */
		/* { */
		LOG_DBG("[rtbc][ENQUE]:buffer_exist(%d),i(%d),PA(0x%x),bFilled(%d),empty(%d)",
			buffer_exist, i, g_rt_buf_info.base_pAddr,
			g_pstRTBuf->ring_buf[rt_dma].data[i].bFilled,
			g_pstRTBuf->ring_buf[rt_dma].empty_count);
		/* } */

	} else {
		/* overwrite oldest element if buffer is full */
		if (g_pstRTBuf->ring_buf[rt_dma].total_count == ISP_RT_BUF_SIZE) {
			LOG_ERR("[rtbc][ENQUE]:buffer full(%d)",
				g_pstRTBuf->ring_buf[rt_dma].total_count);
		} else {
			/* first time add */
			index = g_pstRTBuf->ring_buf[rt_dma].total_count % ISP_RT_BUF_SIZE;

			g_pstRTBuf->ring_buf[rt_dma].data[index].memID = g_rt_buf_info.memID;
			g_pstRTBuf->ring_buf[rt_dma].data[index].size = g_rt_buf_info.size;
			g_pstRTBuf->ring_buf[rt_dma].data[index].base_vAddr =
			    g_rt_buf_info.base_vAddr;
			g_pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr =
			    g_rt_buf_info.base_pAddr;
			g_pstRTBuf->ring_buf[rt_dma].data[index].bFilled = ISP_RTBC_BUF_EMPTY;

			g_pstRTBuf->ring_buf[rt_dma].total_count++;
			g_pstRTBuf->ring_buf[rt_dma].empty_count++;

			/* if(g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) */
			/* { */
			LOG_DBG("[rtbc][ENQUE]:dma(%d),index(%d),PA(0x%x),empty(%d),total(%d)",
				rt_dma, index, g_pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr,
				g_pstRTBuf->ring_buf[rt_dma].empty_count,
				g_pstRTBuf->ring_buf[rt_dma].total_count);
			/* } */
		}
	}

	/* count ==1 means DMA stalled already or NOT start yet */
	if (1 == g_pstRTBuf->ring_buf[rt_dma].empty_count) {
		if (_imgo_ == rt_dma) {
			/* set base_addr at beginning before VF_EN */
			ISP_WR32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR,
				 g_pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr);
		} else {
			/* set base_addr at beginning before VF_EN */
			ISP_WR32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR,
				 g_pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr);
		}

		/* if(g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) */
		/* { */
		LOG_DBG
		    ("[rtbc][ENQUE]:dma(%d),base_pAddr(0x%x),imgo(0x%x),img2o(0x%x),empty_count(%d)",
		     rt_dma, g_pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr,
		     ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR),
		     ISP_RD32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR),
		     g_pstRTBuf->ring_buf[rt_dma].empty_count);
		/* } */

#if defined(_rtbc_use_cq0c_)
		/* Do nothing */
#else
		MUINT32 reg_val = 0;

		/* disable FBC control to go on download */
		if (_imgo_ == rt_dma) {
			reg_val = ISP_RD32(ISP_REG_ADDR_IMGO_FBC);
			reg_val &= ~0x4000;
			ISP_WR32(ISP_REG_ADDR_IMGO_FBC, reg_val);
		} else {
			reg_val = ISP_RD32(ISP_REG_ADDR_IMG2O_FBC);
			reg_val &= ~0x4000;
			ISP_WR32(ISP_REG_ADDR_IMG2O_FBC, reg_val);
		}

		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG("[rtbc][ENQUE]:dma(%d),disable fbc:IMGO(0x%x),IMG2O(0x%x)", rt_dma,
				ISP_RD32(ISP_REG_ADDR_IMGO_FBC), ISP_RD32(ISP_REG_ADDR_IMG2O_FBC));
		}
#endif

		g_pstRTBuf->ring_buf[rt_dma].pre_empty_count =
		    g_pstRTBuf->ring_buf[rt_dma].empty_count;
	}

	/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

	if (g_IspInfo.DebugMask & ISP_DBG_INT) {
		LOG_DBG
		    ("[rtbc][ENQUE]:dma:(%d),start(%d),index(%d),empty_count(%d),base_pAddr(0x%x)",
		     rt_dma, g_pstRTBuf->ring_buf[rt_dma].start, index,
		     g_pstRTBuf->ring_buf[rt_dma].empty_count,
		     g_pstRTBuf->ring_buf[rt_dma].data[index].base_pAddr);
	}

	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_RTBC_DEQUE(MINT32 dma)
{
	MINT32 Ret = 0;
	MINT32 rt_dma = dma;
	MUINT32 i = 0;
	MUINT32 index = 0;

	/* spin_lock_irqsave(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("[rtbc][DEQUE]+");
	}
	g_deque_buf.count = 0;

	/* in SOF, "start" is next buffer index */
	for (i = 0; i < g_pstRTBuf->ring_buf[rt_dma].total_count; i++) {
		index =
		    (g_pstRTBuf->ring_buf[rt_dma].start +
		     i) % g_pstRTBuf->ring_buf[rt_dma].total_count;

		if (ISP_RTBC_BUF_FILLED == g_pstRTBuf->ring_buf[rt_dma].data[index].bFilled) {
			g_pstRTBuf->ring_buf[rt_dma].data[index].bFilled = ISP_RTBC_BUF_LOCKED;
			g_deque_buf.count = 1;
			break;
		}
	}

	if (0 == g_deque_buf.count) {
		/* queue buffer status */
		LOG_DBG
		    ("[rtbc][DEQUE]:dma(%d),start(%d),total(%d),empty(%d), g_deque_buf.count(%d)",
		     rt_dma, g_pstRTBuf->ring_buf[rt_dma].start,
		     g_pstRTBuf->ring_buf[rt_dma].total_count,
		     g_pstRTBuf->ring_buf[rt_dma].empty_count, g_deque_buf.count);

		for (i = 0; i <= g_pstRTBuf->ring_buf[rt_dma].total_count - 1; i++) {
			LOG_DBG
			    ("[rtbc][DEQUE]Buf List:i(%d),memID(%d),size(0x%x),VA(0x%llx),PA(0x%x),bFilled(%d)",
			     i, g_pstRTBuf->ring_buf[rt_dma].data[i].memID,
			     g_pstRTBuf->ring_buf[rt_dma].data[i].size,
			     g_pstRTBuf->ring_buf[rt_dma].data[i].base_vAddr,
			     g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr,
			     g_pstRTBuf->ring_buf[rt_dma].data[i].bFilled);
		}
	}

	if (g_deque_buf.count) {
		/* Fill buffer head */
		/* "start" is current working index */
		if (g_IspInfo.DebugMask & ISP_DBG_INT) {
			LOG_DBG("[rtbc][DEQUE]:rt_dma(%d),index(%d),empty(%d),total(%d)", rt_dma,
				index,
				g_pstRTBuf->ring_buf[rt_dma].empty_count,
				g_pstRTBuf->ring_buf[rt_dma].total_count);
		}

		for (i = 0; i < g_deque_buf.count; i++) {
			g_deque_buf.data[i].memID =
			    g_pstRTBuf->ring_buf[rt_dma].data[index + i].memID;
			g_deque_buf.data[i].size =
			    g_pstRTBuf->ring_buf[rt_dma].data[index + i].size;
			g_deque_buf.data[i].base_vAddr =
			    g_pstRTBuf->ring_buf[rt_dma].data[index + i].base_vAddr;
			g_deque_buf.data[i].base_pAddr =
			    g_pstRTBuf->ring_buf[rt_dma].data[index + i].base_pAddr;
			g_deque_buf.data[i].timeStampS =
			    g_pstRTBuf->ring_buf[rt_dma].data[index + i].timeStampS;
			g_deque_buf.data[i].timeStampUs =
			    g_pstRTBuf->ring_buf[rt_dma].data[index + i].timeStampUs;

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
				LOG_DBG
				    ("[rtbc][DEQUE]:index(%d),PA(0x%x),memID(%d),size(0x%x),VA(0x%llx)",
				     index + i, g_deque_buf.data[i].base_pAddr,
				     g_deque_buf.data[i].memID, g_deque_buf.data[i].size,
				     g_deque_buf.data[i].base_vAddr);
			}

		}

		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG("[rtbc][DEQUE]-");
		}

		/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */
	} else {
		/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */
		LOG_ERR("[rtbc][DEQUE]:no filled buffer");
		Ret = -EFAULT;
	}

	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
/* static long ISP_Buf_CTRL_FUNC(MUINT32 Param) */
static long ISP_Buf_CTRL_FUNC(unsigned long Param)
{
	MINT32 Ret = 0;
	MINT32 rt_dma;
	MUINT32 reg_val = 0;
	MUINT32 reg_val2 = 0;
	MUINT32 i = 0;
	MUINT32 iBuf = 0;
	MUINT32 size = 0;
	MUINT32 bWaitBufRdy = 0;
	ISP_BUFFER_CTRL_STRUCT rt_buf_ctrl;
	/* MUINT32 buffer_exist = 0; */

#if defined(_rtbc_use_cq0c_)	/* fix compile warning */

	CQ_RTBC_FBC imgo_fbc;
	CQ_RTBC_FBC img2o_fbc;

	volatile MUINT32 curr_pa = 0;
#else

	MINT32 Timeout = 1000;	/* ms */

#endif


	if (NULL == g_pstRTBuf) {
		LOG_ERR("[rtbc]NULL g_pstRTBuf");
		return -EFAULT;
	}

	if (copy_from_user(&rt_buf_ctrl, (void __user *)Param, sizeof(ISP_BUFFER_CTRL_STRUCT)) == 0) {
		rt_dma = rt_buf_ctrl.buf_id;

		if (g_IspInfo.DebugMask & ISP_DBG_INT) {
			LOG_DBG("[rtbc]ctrl(0x%x),buf_id(0x%x),data_ptr(0x%p),ex_data_ptr(0x%p)",
				rt_buf_ctrl.ctrl, rt_buf_ctrl.buf_id, rt_buf_ctrl.data_ptr,
				rt_buf_ctrl.ex_data_ptr);
		}

		if (_imgo_ != rt_dma && _img2o_ != rt_dma) {
			LOG_ERR("[rtbc]invalid dma channel(%d)", rt_dma);
			return -EFAULT;
		}
#if defined(_rtbc_use_cq0c_)

		if ((ISP_RT_BUF_CTRL_ENQUE == rt_buf_ctrl.ctrl) ||
		    (ISP_RT_BUF_CTRL_EXCHANGE_ENQUE == rt_buf_ctrl.ctrl) ||
		    (ISP_RT_BUF_CTRL_DEQUE == rt_buf_ctrl.ctrl) ||
		    (ISP_RT_BUF_CTRL_IS_RDY == rt_buf_ctrl.ctrl)) {
			imgo_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC);
			img2o_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC);

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
				LOG_DBG("[rtbc]:ctrl(%d),o(0x%x),2o(0x%x)", rt_buf_ctrl.ctrl,
					ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC),
					ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC));
			}
		}
#endif

		switch (rt_buf_ctrl.ctrl) {
		case ISP_RT_BUF_CTRL_ENQUE:
		case ISP_RT_BUF_CTRL_EXCHANGE_ENQUE:
			if (copy_from_user
			    (&g_rt_buf_info, (void __user *)rt_buf_ctrl.data_ptr,
			     sizeof(ISP_RT_BUF_INFO_STRUCT)) == 0) {
				reg_val = ISP_RD32((void *)ISP_REG_ADDR_TG_VF_CON);
				/* reg_val2 = ISP_RD32(ISP_REG_ADDR_TG2_VF_CON); */

				/* VF start already */
				if ((reg_val & 0x01) || (reg_val2 & 0x01)) {
#if defined(_rtbc_use_cq0c_)

					if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
						LOG_DBG("[rtbc][ENQUE]:ex_data_ptr(0x%p)",
							rt_buf_ctrl.ex_data_ptr);
					}

					if (0 != rt_buf_ctrl.ex_data_ptr) {
					/**/	if (copy_from_user
						    (&g_ex_rt_buf_info,
						     (void __user *)rt_buf_ctrl.ex_data_ptr,
						     sizeof(ISP_RT_BUF_INFO_STRUCT)) == 0) {
					/**/		for (i = 0; i < ISP_RT_BUF_SIZE; i++) {
					/**/			if (g_pstRTBuf->ring_buf[rt_dma].
								    data[i].base_pAddr ==
								    g_rt_buf_info.base_pAddr) {
									g_oldImgoAddr =
									    g_rt_buf_info.
									    base_pAddr;
									LOG_DBG
									    ("[rtbc]dma(%d),old(%d),PA(0x%x),VA(0x%llx)",
									     rt_dma, i,
									     g_pstRTBuf->
									     ring_buf[rt_dma].
									     data[i].base_pAddr,
									     g_pstRTBuf->
									     ring_buf[rt_dma].
									     data[i].base_vAddr);

									g_newImgoAddr =
									    g_ex_rt_buf_info.
									    base_pAddr;
									g_pstRTBuf->
									    ring_buf[rt_dma].
									    data[i].memID =
									    g_ex_rt_buf_info.memID;
									g_pstRTBuf->
									    ring_buf[rt_dma].
									    data[i].size =
									    g_ex_rt_buf_info.size;
									g_pstRTBuf->
									    ring_buf[rt_dma].
									    data[i].base_pAddr =
									    g_ex_rt_buf_info.
									    base_pAddr;
									g_pstRTBuf->
									    ring_buf[rt_dma].
									    data[i].base_vAddr =
									    g_ex_rt_buf_info.
									    base_vAddr;

									LOG_DBG
									    ("[rtbc]dma(%d),new(%d),PA(0x%x),VA(0x%llx)",
									     rt_dma, i,
									     g_pstRTBuf->
									     ring_buf[rt_dma].
									     data[i].base_pAddr,
									     g_pstRTBuf->
									     ring_buf[rt_dma].
									     data[i].base_vAddr);

									/* set imgo exchange buffer address to HW */
					/**/				if (rt_dma == _imgo_) {
										curr_pa =
										    ISP_RD32((void
											      *)
											     ISP_REG_ADDR_IMGO_BASE_ADDR);
										LOG_DBG
										    ("[rtbc][EXG]o,curr_pa(0x%x),old(0x%x)",
										     curr_pa,
										     g_rt_buf_info.
										     base_pAddr);
										LOG_DBG
										    ("[rtbc][EXG]o,NUM(0x%x),CNT(0x%x)",
										     imgo_fbc.Bits.
										     FB_NUM,
										     imgo_fbc.Bits.
										     FBC_CNT);

					/**/					if ((curr_pa ==
										     g_rt_buf_info.
										     base_pAddr)
										    && (imgo_fbc.
											Bits.
											FB_NUM != 0)
										    &&
										    ((imgo_fbc.Bits.
										      FB_NUM ==
										      imgo_fbc.Bits.
										      FBC_CNT)
										     || (imgo_fbc.
											 Bits.
											 FBC_CNT ==
											 (imgo_fbc.
											  Bits.
											  FB_NUM -
											  1)))) {
											ISP_WR32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR, g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr);
					/**/					} else {	/* timing issue */

											g_oldImgoAddr
											    =
											    g_rt_buf_info.
											    base_pAddr;
											g_newImgoAddr
											    =
											    g_pstRTBuf->
											    ring_buf
											    [rt_dma].
											    data[i].
											    base_pAddr;
											LOG_DBG
											    ("[rtbc][EXG]o,g_old(0x%x),g_new(0x%x)",
											     g_oldImgoAddr,
											     g_newImgoAddr);
										}
					/**/				} else if (rt_dma ==
										   _img2o_) {
										curr_pa =
										    ISP_RD32((void
											      *)
											     ISP_REG_ADDR_IMG2O_BASE_ADDR);
										LOG_DBG
										    ("[rtbc][EXG]2o,curr_pa(0x%x),old(0x%x)",
										     curr_pa,
										     g_rt_buf_info.
										     base_pAddr);
										LOG_DBG
										    ("[rtbc][EXG]2o,NUM(0x%x),CNT(0x%x)",
										     img2o_fbc.Bits.
										     FB_NUM,
										     img2o_fbc.Bits.
										     FBC_CNT);

					/**/					if ((curr_pa ==
										     g_rt_buf_info.
										     base_pAddr)
										    && (img2o_fbc.
											Bits.
											FB_NUM != 0)
										    &&
										    ((img2o_fbc.
										      Bits.FB_NUM ==
										      img2o_fbc.
										      Bits.FBC_CNT)
										     || (img2o_fbc.
											 Bits.
											 FBC_CNT ==
											 (img2o_fbc.
											  Bits.
											  FB_NUM -
											  1)))) {
											ISP_WR32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR, g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr);
					/**/					} else {	/* timing issue */

											g_oldImg2oAddr
											    =
											    g_rt_buf_info.
											    base_pAddr;
											g_newImg2oAddr
											    =
											    g_pstRTBuf->
											    ring_buf
											    [rt_dma].
											    data[i].
											    base_pAddr;
											LOG_DBG
											    ("[rtbc][EXG]2o,g_old(0x%x),g_new(0x%x)",
											     g_oldImg2oAddr,
											     g_newImg2oAddr);
										}
									}

									break;
								}
							}
						}
					}

					/* set RCN_INC = 1; */
					/* RCNT++ */
					/* FBC_CNT-- */
					if (_imgo_ == rt_dma) {
						imgo_fbc.Bits.RCNT_INC = 1;
						ISP_WR32((void *)ISP_REG_ADDR_IMGO_FBC,
							 imgo_fbc.Reg_val);
					} else if (_img2o_ == rt_dma) {
						img2o_fbc.Bits.RCNT_INC = 1;
						ISP_WR32((void *)ISP_REG_ADDR_IMG2O_FBC,
							 img2o_fbc.Reg_val);
					}

					/* if(g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) */
					/* { */
					LOG_DBG
					    ("[rtbc][ENQ]:dma(%d),P(0x%x),O(0x%x),2O(0x%x),fps(%d)us",
					     rt_dma, g_rt_buf_info.base_pAddr,
					     ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR),
					     ISP_RD32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR),
					     g_avg_frame_time);
					/* } */
#else


					if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
						LOG_DBG
						    ("[rtbc][ENQUE]+:wait enque done.(0x%x)(0x%x)",
						     reg_val, reg_val2);
					}

					/* wait till enq done in SOF */
					g_rtbc_enq_dma = rt_dma;
					g_EnqBuf = 1;

					Timeout =
					    wait_event_interruptible_timeout(g_IspInfo.
									     WaitQueueHead,
									     (0 == g_EnqBuf),
									     ISP_MsToJiffies
									     (Timeout));

					if (Timeout == 0) {
						LOG_ERR("[rtbc][ENQUE]:timeout(%d)", g_EnqBuf);
						Ret = -EFAULT;
					}

					if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
						LOG_DBG("[rtbc][ENQUE]-:enque done");
					}
#endif
				} else {
					ISP_RTBC_ENQUE(rt_dma);
				}
			} else {
				LOG_ERR("[rtbc][ENQUE]:copy_from_user fail");
				return -EFAULT;
			}
			break;
		case ISP_RT_BUF_CTRL_DEQUE:
			reg_val = ISP_RD32((void *)ISP_REG_ADDR_TG_VF_CON);
			/* reg_val2 = ISP_RD32(ISP_REG_ADDR_TG2_VF_CON); */

			/* VF start already */
			if ((reg_val & 0x01) || (reg_val2 & 0x01)) {

#if defined(_rtbc_use_cq0c_)

				g_deque_buf.count = 1;

				iBuf =
				    ((_imgo_ ==
				      rt_dma) ? imgo_fbc.Bits.RCNT : img2o_fbc.Bits.RCNT) - 1;

				for (i = 0; i < g_deque_buf.count; i++) {
					g_deque_buf.data[i].memID =
					    g_pstRTBuf->ring_buf[rt_dma].data[iBuf + i].memID;
					g_deque_buf.data[i].size =
					    g_pstRTBuf->ring_buf[rt_dma].data[iBuf + i].size;
					g_deque_buf.data[i].base_vAddr =
					    g_pstRTBuf->ring_buf[rt_dma].data[iBuf + i].base_vAddr;
					g_deque_buf.data[i].base_pAddr =
					    g_pstRTBuf->ring_buf[rt_dma].data[iBuf + i].base_pAddr;
					g_deque_buf.data[i].timeStampS =
					    g_pstRTBuf->ring_buf[rt_dma].data[iBuf + i].timeStampS;
					g_deque_buf.data[i].timeStampUs =
					    g_pstRTBuf->ring_buf[rt_dma].data[iBuf + i].timeStampUs;

					/* if(g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) */
					/* { */
					LOG_DBG
					    ("[rtbc][DEQ]:T\"%d.%06d\",co(0x%08lx),c2o(0x%08lx),i(%d),iBuf+i(%d),V(0x%llx),P(0x%x),O(0x%x),2O(0x%x)",
					     g_deque_buf.data[i].timeStampS,
					     g_deque_buf.data[i].timeStampUs, imgo_fbc.Reg_val,
					     img2o_fbc.Reg_val, i, iBuf + i,
					     g_deque_buf.data[i].base_vAddr,
					     g_deque_buf.data[i].base_pAddr,
					     ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR),
					     ISP_RD32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR));
					/* } */

					/* tstamp = g_deque_buf.data[i].timeStampS*1000000+g_deque_buf.data[i].timeStampUs; */
					/* if ( (0 != prv_tstamp) && (prv_tstamp >= tstamp) ) */
					/* { */
					if (0 != g_prv_tstamp_s) {
					/**/	if ((g_prv_tstamp_s >
						     g_deque_buf.data[i].timeStampS)
						    ||
						    ((g_prv_tstamp_s ==
						      g_deque_buf.data[i].timeStampS)
						     && (g_prv_tstamp_us >=
							 g_deque_buf.data[i].timeStampUs))) {
							LOG_ERR
							    ("[rtbc]TS rollback,prv\"%d.%06d\",cur\"%d.%06d\"",
							     g_prv_tstamp_s, g_prv_tstamp_us,
							     g_deque_buf.data[i].timeStampS,
							     g_deque_buf.data[i].timeStampUs);
						}
					}

					g_prv_tstamp_s = g_deque_buf.data[i].timeStampS;
					g_prv_tstamp_us = g_deque_buf.data[i].timeStampUs;
					/* } */
				}

#if 0
				LOG_DBG
				    ("+LARB in DEQUE,BWL(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X),220(0x%08X)/(0x%08X),0x14(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)/(0x%08X)",
				     ISP_RD32(0xF0202000 + 0x204), ISP_RD32(0xF0202000 + 0x20c),
				     ISP_RD32(0xF0202000 + 0x210), ISP_RD32(0xF0202000 + 0x214),
				     ISP_RD32(0xF0202000 + 0x220), ISP_RD32(0xF0202000 + 0x230),
				     ISP_RD32(SMI_LARB0 + 0x10), ISP_RD32(SMI_LARB1 + 0x10),
				     ISP_RD32(SMI_LARB2 + 0x10), ISP_RD32(SMI_LARB3 + 0x10),
				     ISP_RD32(SMI_LARB4 + 0x10));
#endif

#else

				if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
					LOG_DBG("[rtbc][DEQUE]+:wait deque done.(0x%x)(0x%x)",
						reg_val, reg_val2);
				}

				/* wait till deq done in SOF */
				g_rtbc_deq_dma = rt_dma;
				g_DeqBuf = 1;

				Timeout = wait_event_interruptible_timeout(g_IspInfo.WaitQueueHead,
									   (0 == g_DeqBuf),
									   ISP_MsToJiffies
									   (Timeout));

				if (Timeout == 0) {
					LOG_ERR("[rtbc][DEQUE]:timeout(%d)", g_DeqBuf);
					LOG_ERR("ISP_IRQ_TYPE_INT:IrqStatus(0x%08X)",
						g_IspInfo.IrqInfo.Status[ISP_IRQ_TYPE_INT]);
					ISP_DumpReg();
					Ret = -EFAULT;
				}

				if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
					LOG_DBG("[rtbc][DEQUE]-:deque done");
				}
#endif
			} else {
				ISP_RTBC_DEQUE(rt_dma);
			}

			if (g_deque_buf.count) {
				if (copy_to_user
				    ((void __user *)rt_buf_ctrl.pExtend, &g_deque_buf,
				     sizeof(ISP_DEQUE_BUF_INFO_STRUCT)) != 0) {
					LOG_ERR("[rtbc][DEQUE]:copy_to_user failed");
					Ret = -EFAULT;
				}
			} else {
				/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */
				LOG_ERR("[rtbc][DEQUE]:no filled buffer");
				Ret = -EFAULT;
			}
			break;
		case ISP_RT_BUF_CTRL_IS_RDY:
			/* spin_lock_irqsave(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

			bWaitBufRdy = 1;

#if defined(_rtbc_use_cq0c_)
			bWaitBufRdy =
			    ((_imgo_ ==
			      rt_dma) ? (imgo_fbc.Bits.FBC_CNT) : (img2o_fbc.Bits.FBC_CNT)) ? 0 : 1;
#else
			for (i = 0; i <= g_pstRTBuf->ring_buf[rt_dma].total_count; i++) {
				if (ISP_RTBC_BUF_FILLED ==
				    g_pstRTBuf->ring_buf[rt_dma].data[i].bFilled) {
					bWaitBufRdy = 0;
					break;
				}
			}
#endif

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
				LOG_DBG("[rtbc][IS_RDY]:bWaitBufRdy(%d)", bWaitBufRdy);
			}


			/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

			if (copy_to_user
			    ((void __user *)rt_buf_ctrl.pExtend, &bWaitBufRdy,
			     sizeof(MUINT32)) != 0) {
				LOG_ERR("[rtbc][IS_RDY]:copy_to_user failed");
				Ret = -EFAULT;
			}

			/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC), flags); */
			break;
		case ISP_RT_BUF_CTRL_GET_SIZE:
			size = g_pstRTBuf->ring_buf[rt_dma].total_count;

			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG("[rtbc][GET_SIZE]:rt_dma(%d)/size(%d)", rt_dma, size);
			}

			if (copy_to_user((void __user *)rt_buf_ctrl.pExtend, &size, sizeof(MUINT32))
			    != 0) {
				LOG_ERR("[rtbc][GET_SIZE]:copy_to_user failed");
				Ret = -EFAULT;
			}
			break;
		case ISP_RT_BUF_CTRL_CLEAR:

			/* if(g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) */
			/* { */
			LOG_DBG("[rtbc][CLEAR]:rt_dma(%d)", rt_dma);
			/* } */

#if 0
			g_pstRTBuf->ring_buf[rt_dma].total_count = 0;
			g_pstRTBuf->ring_buf[rt_dma].start = 0;
			g_pstRTBuf->ring_buf[rt_dma].empty_count = 0;
			g_pstRTBuf->ring_buf[rt_dma].active = 0;

			for (i = 0; i < ISP_RT_BUF_SIZE; i++) {
				if (g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr ==
				    g_rt_buf_info.base_pAddr) {
					buffer_exist = 1;
					break;
				}
				/*  */
				if (g_pstRTBuf->ring_buf[rt_dma].data[i].base_pAddr == 0) {
					break;
				}
			}
#else
			memset((char *)g_pstRTBuf, 0x00, sizeof(ISP_RT_BUF_STRUCT));
			g_prv_tstamp_s = 0;
			g_prv_tstamp_us = 0;

			g_sof_count = 0;
			g_start_time = 0;
			g_avg_frame_time = 0;

			g_TempAddr = 0;

			g_oldImgoAddr = DEFAULT_PA;
			g_newImgoAddr = DEFAULT_PA;
			g_oldImg2oAddr = DEFAULT_PA;
			g_newImg2oAddr = DEFAULT_PA;

#endif
			break;
		case ISP_RT_BUF_CTRL_MAX:	/* Add this to remove build warning */
			break;	/* Do nothing */
		default:
			LOG_ERR("[rtbc] error ctrl(%d)", rt_buf_ctrl.ctrl);
			break;
		}
	} else {
		LOG_ERR("[rtbc]copy_from_user failed");
		Ret = -EFAULT;
	}

	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_SOF_Buf_Get(unsigned long long sec, unsigned long usec)
{
#if defined(_rtbc_use_cq0c_)

	CQ_RTBC_FBC imgo_fbc;
	CQ_RTBC_FBC img2o_fbc;
	MUINT32 curr_imgo = 0;	/* (imgo_fbc.Bits.WCNT+imgo_fbc.Bits.FB_NUM-1)%imgo_fbc.Bits.FB_NUM; //[0,1,2,...] */
	MUINT32 curr_img2o = 0;	/* (img2o_fbc.Bits.WCNT+img2o_fbc.Bits.FB_NUM-1)%img2o_fbc.Bits.FB_NUM; //[0,1,2,...] */

	volatile MUINT32 curr_pa = 0;
	MUINT32 i = 0;

	/* ====== Read FBC Register Value ====== */

	imgo_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC);
	img2o_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC);

	/* ====== Check Drop Frame Or Not ====== */

	if (imgo_fbc.Bits.FB_NUM != imgo_fbc.Bits.FBC_CNT) {	/* No drop */
		g_pstRTBuf->dropCnt = 0;
	} else {		/* dropped */

		g_pstRTBuf->dropCnt = 1;
	}

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("[rtbc]dropCnt(%d)", (int)(g_pstRTBuf->dropCnt));
	}

	/* No drop */
	if (0 == g_pstRTBuf->dropCnt) {
		/* verify write buffer */
		curr_pa = ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR);
		if (g_TempAddr == curr_pa) {
			LOG_DBG("[WARNING]g_TempAddr:Last(0x%08X) == Cur(0x%08X)", g_TempAddr,
				curr_pa);
			/* ISP_DumpReg(); */
		}

		g_TempAddr = curr_pa;

		/* last update buffer index */
		curr_imgo = imgo_fbc.Bits.WCNT - 1;	/* [0,1,2,...] */
		/* curr_img2o = img2o_fbc.Bits.WCNT - 1; //[0,1,2,...] */
		curr_img2o = curr_imgo;

#if 1

		/* verify write buffer,once pass1_done lost, WCNT is untrustful. */
		if (ISP_RT_CQ0C_BUF_SIZE < g_pstRTBuf->ring_buf[_imgo_].total_count) {
			LOG_ERR("[rtbc]buf cnt(%d)", g_pstRTBuf->ring_buf[_imgo_].total_count);
			g_pstRTBuf->ring_buf[_imgo_].total_count = ISP_RT_CQ0C_BUF_SIZE;
		}

		if (curr_pa != g_pstRTBuf->ring_buf[_imgo_].data[curr_imgo].base_pAddr) {
			for (i = 0; i < g_pstRTBuf->ring_buf[_imgo_].total_count; i++) {
				if (curr_pa == g_pstRTBuf->ring_buf[_imgo_].data[i].base_pAddr) {
					if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
						LOG_DBG("[rtbc]curr:old/new(%d/%d)", curr_imgo, i);
					}

					curr_imgo = i;
					curr_img2o = i;
					break;
				}
			}
		}
#endif

		g_pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampS = sec;
		g_pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampUs = usec;
		g_pstRTBuf->ring_buf[_img2o_].data[curr_img2o].timeStampS = sec;
		g_pstRTBuf->ring_buf[_img2o_].data[curr_img2o].timeStampUs = usec;

		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG
			    ("[rtbc]TStamp(%d.%06d),curr(%d),pa(0x%x/0x%x),o2o(0x%x/0x%x),fbc_o(0x%08x),fbc_2o(0x%08x),cq0c(0x%x)",
			     g_pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampS,
			     g_pstRTBuf->ring_buf[_imgo_].data[curr_imgo].timeStampUs, curr_imgo,
			     g_pstRTBuf->ring_buf[_imgo_].data[curr_imgo].base_pAddr,
			     g_pstRTBuf->ring_buf[_img2o_].data[curr_imgo].base_pAddr,
			     ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR),
			     ISP_RD32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR),
			     (unsigned int)(imgo_fbc.Reg_val), (unsigned int)(img2o_fbc.Reg_val),
			     ISP_RD32((void *)ISP_ADDR + 0xBC));
		}
	}

	/* frame time profile */
	if (0 == g_sof_count) {
		g_start_time = sec * 1000000 + usec;
	}

	/* calc once per senond */
	if (1000000 < ((sec * 1000000 + usec) - g_start_time)) {
		if (0 != g_sof_count) {
			g_avg_frame_time = (sec * 1000000 + usec) - g_start_time;
			g_avg_frame_time = (MUINT32) (g_avg_frame_time / g_sof_count);
		}
		g_sof_count = 0;
	} else {
		g_sof_count++;
	}

#else

	MINT32 i, i_dma;
	MUINT32 pAddr = 0;
	MUINT32 dma_base_addr = 0;
	MUINT32 next = 0;

	if (g_IspInfo.DebugMask & ISP_DBG_INT) {
		LOG_DBG("[rtbc]E:fbc(0x%x/0x%x)/g_rtbcAAA(%d)",
			ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC),
			ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC), g_rtbcAAA++);
	}

	/* spin_lock_irqsave(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("+:[rtbc]g_rtbcAAA(%d)", g_rtbcAAA++);
	}

	for (i = 0; i <= 1; i++) {
		i_dma = (0 == i) ? _imgo_ : _img2o_;
		dma_base_addr =
		    (_imgo_ == i_dma) ? ISP_REG_ADDR_IMGO_BASE_ADDR : ISP_REG_ADDR_IMG2O_BASE_ADDR;

		/* queue buffer status */
		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG("[rtbc][SOF]:dma(%d),start(%d),total(%d),empty(%d) ",
				i_dma,
				g_pstRTBuf->ring_buf[i_dma].start,
				g_pstRTBuf->ring_buf[i_dma].total_count,
				g_pstRTBuf->ring_buf[i_dma].empty_count);

			for (i = 0; i <= g_pstRTBuf->ring_buf[i_dma].total_count - 1; i++) {
				LOG_DBG("[rtbc][SOF]Buf List:%d/%d/0x%x/0x%x/0x%x/%d/  ",
					i,
					g_pstRTBuf->ring_buf[i_dma].data[i].memID,
					g_pstRTBuf->ring_buf[i_dma].data[i].size,
					g_pstRTBuf->ring_buf[i_dma].data[i].base_vAddr,
					g_pstRTBuf->ring_buf[i_dma].data[i].base_pAddr,
					g_pstRTBuf->ring_buf[i_dma].data[i].bFilled);
			}
		}

		/* ring buffer get next buffer */
		if (0 == g_pstRTBuf->ring_buf[i_dma].empty_count) {
			/* once if buffer put into queue between SOF and ISP_DONE. */
			g_pstRTBuf->ring_buf[i_dma].active = MFALSE;

			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG
				    ("[rtbc][SOF]:dma(%d)real time buffer number empty,start(%d) ",
				     i_dma, g_pstRTBuf->ring_buf[i_dma].start);
			}
		} else {
			if (2 <= g_pstRTBuf->ring_buf[i_dma].empty_count) {
				/* next buffer */
				next =
				    (g_pstRTBuf->ring_buf[i_dma].start +
				     1) % g_pstRTBuf->ring_buf[i_dma].total_count;
				pAddr = g_pstRTBuf->ring_buf[i_dma].data[next].base_pAddr;

				ISP_WR32(dma_base_addr, pAddr);

				if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
					LOG_DBG
					    ("[rtbc][SOF]:dma(%d),start(%d),empty(%d),next(%d),next_PA(0x%x) ",
					     i_dma, g_pstRTBuf->ring_buf[i_dma].start,
					     g_pstRTBuf->ring_buf[i_dma].empty_count, next, pAddr);
				}
			} else {
				if (g_IspInfo.DebugMask & ISP_DBG_INT) {
					LOG_DBG
					    ("[rtbc][SOF]:dma(%d)real time buffer number is running out ",
					     i_dma);
				}
			}

			/* once if buffer put into queue between SOF and ISP_DONE. */
			g_pstRTBuf->ring_buf[i_dma].active = MTRUE;
		}
	}

	if (g_EnqBuf) {
		ISP_RTBC_ENQUE(g_rtbc_enq_dma);
		g_EnqBuf = 0;
		wake_up_interruptible(&g_IspInfo.WaitQueueHead);
	}

	if (g_DeqBuf) {
		ISP_RTBC_DEQUE(g_rtbc_deq_dma);
		g_DeqBuf = 0;
		wake_up_interruptible(&g_IspInfo.WaitQueueHead);
	}

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("-:[rtbc]");
	}

	g_pstRTBuf->state = ISP_RTBC_STATE_SOF;

	/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */
#endif

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
#ifndef _rtbc_use_cq0c_
static MINT32 ISP_DONE_Buf_Time(unsigned long long sec, unsigned long usec)
{
	MINT32 i, i_dma;
	MUINT32 curr;
	MUINT32 reg_fbc;
	MUINT32 reg_val = 0;

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("[rtbc]E:fbc(0x%x/0x%x)", ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC),
			ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC));
	}
#if 0
	if (spin_trylock_bh(&(g_IspInfo.SpinLockRTBC))) {
		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG("[rtbc]:unlock state");
		}
	} else {
		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG("[rtbc]:locked state");
		}
	}
#endif

	/* spin_lock_irqsave(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("+:[rtbc]g_rtbcAAA(%d)", g_rtbcAAA++);
	}

	for (i = 0; i <= 1; i++) {
		i_dma = (0 == i) ? _imgo_ : _img2o_;
		reg_fbc = (_imgo_ == i_dma) ? ISP_REG_ADDR_IMGO_FBC : ISP_REG_ADDR_IMG2O_FBC;

		if (0 == g_pstRTBuf->ring_buf[i_dma].empty_count) {
			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG
				    ("[rtbc][DONE]:dma(%d)real time buffer number empty,start(%d)",
				     i_dma, g_pstRTBuf->ring_buf[i_dma].start);
			}

			continue;
		}

		/* once if buffer put into queue between SOF and ISP_DONE. */
		if (MFALSE == g_pstRTBuf->ring_buf[i_dma].active) {
			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG("[rtbc][DONE] ERROR: missing SOF");
			}

			continue;
		}

		while (1) {
			MUINT32 loopCount = 0;

			curr = g_pstRTBuf->ring_buf[i_dma].start;

			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG("i_dma(%d),curr(%d),bFilled(%d)", i_dma,
					curr, g_pstRTBuf->ring_buf[i_dma].data[curr].bFilled);
			}

			if (g_pstRTBuf->ring_buf[i_dma].data[curr].bFilled == ISP_RTBC_BUF_EMPTY) {
				g_pstRTBuf->ring_buf[i_dma].data[curr].bFilled =
				    ISP_RTBC_BUF_FILLED;

				/* start + 1 */
				g_pstRTBuf->ring_buf[i_dma].start =
				    (curr + 1) % g_pstRTBuf->ring_buf[i_dma].total_count;
				g_pstRTBuf->ring_buf[i_dma].empty_count--;
				break;
			} else {
				if (g_IspInfo.DebugMask & ISP_DBG_INT) {
					LOG_DBG
					    ("i_dma(%d),curr(%d),bFilled(%d) != ISP_RTBC_BUF_EMPTY",
					     i_dma, curr,
					     g_pstRTBuf->ring_buf[i_dma].data[curr].bFilled);
				}

				/* start + 1 */
				g_pstRTBuf->ring_buf[i_dma].start =
				    (curr + 1) % g_pstRTBuf->ring_buf[i_dma].total_count;
			}

			loopCount++;
			if (loopCount > g_pstRTBuf->ring_buf[i_dma].total_count) {
				LOG_ERR("Can't find empty dma(%d) buf in total_count(%d)", i_dma,
					g_pstRTBuf->ring_buf[i_dma].total_count);
				break;
			}
		}

		/* enable fbc to stall DMA */
		if (0 == g_pstRTBuf->ring_buf[i_dma].empty_count) {
			if (_imgo_ == i_dma) {
				reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC);
				reg_val |= 0x4000;
				/* ISP_WR32(ISP_REG_ADDR_IMGO_FBC,reg_val); */
			} else {
				reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC);
				reg_val |= 0x4000;
				/* ISP_WR32(ISP_REG_ADDR_IMG2O_FBC,reg_val); */
			}

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
				LOG_DBG("[rtbc][DONE]:dma(%d),en fbc(0x%x) stalled DMA out", i_dma,
					ISP_RD32((void *)reg_fbc));
			}
		}

		if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
			LOG_DBG("[rtbc][DONE]:dma(%d),start(%d),empty(%d)", i_dma,
				g_pstRTBuf->ring_buf[i_dma].start,
				g_pstRTBuf->ring_buf[i_dma].empty_count);
		}

		g_pstRTBuf->ring_buf[i_dma].data[curr].timeStampS = sec;
		g_pstRTBuf->ring_buf[i_dma].data[curr].timeStampUs = usec;

		if (g_IspInfo.DebugMask & ISP_DBG_INT) {
			LOG_DBG("[rtbc][DONE]:dma(%d),curr(%d),sec(%lld),usec(%ld) ", i_dma, curr,
				sec, usec);
		}
	}

	if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
		LOG_DBG("-:[rtbc]");
	}

	g_pstRTBuf->state = ISP_RTBC_STATE_DONE;

	/* spin_unlock_irqrestore(&(g_IspInfo.SpinLockRTBC),g_Flash_SpinLock); */

	return 0;
}

#endif

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_WaitIrq(ISP_WAIT_IRQ_STRUCT WaitIrq)
{
	MINT32 Ret = 0, Timeout = WaitIrq.Timeout;
	MUINT32 i;
	unsigned long flags;
	MUINT32 startTimeS = 0, startTimeUS = 0;
	MUINT32 endTimeS = 0, endTimeUS = 0;
#if 0 /*commend out for prink too much*/
	if (g_IspInfo.DebugMask & ISP_DBG_INT) {
		LOG_DBG("Clear(%d),Type(%d),Status(0x%08X),Timeout(%d)", WaitIrq.Clear,
			WaitIrq.Type, WaitIrq.Status, WaitIrq.Timeout);
	}
#endif
	if (WaitIrq.Clear == ISP_IRQ_CLEAR_WAIT) {
		spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);
		if (g_IspInfo.IrqInfo.Status[WaitIrq.Type] & WaitIrq.Status) {
			#if 0 /*commend out for prink too much*/
			LOG_DBG("WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been cleared",
				WaitIrq.Clear, WaitIrq.Type,
				g_IspInfo.IrqInfo.Status[WaitIrq.Type] & WaitIrq.Status);
			#endif
			g_IspInfo.IrqInfo.Status[WaitIrq.Type] &= (~WaitIrq.Status);
		}
		spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);
	} else if (WaitIrq.Clear == ISP_IRQ_CLEAR_ALL) {
		spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);
		#if 0 /*commend out for prink too much*/
		LOG_DBG("WARNING: Clear(%d), Type(%d): IrqStatus(0x%08X) has been cleared",
			WaitIrq.Clear, WaitIrq.Type, g_IspInfo.IrqInfo.Status[WaitIrq.Type]);
		#endif
		g_IspInfo.IrqInfo.Status[WaitIrq.Type] = 0;
		spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);
	}

#if ISP_IRQ_POLLING

	ISP_IRQ_TYPE_ENUM IrqStatus[ISP_IRQ_TYPE_AMOUNT];

	while (1) {
		IrqStatus[ISP_IRQ_TYPE_INT] =
		    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUS) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_INT]));
		IrqStatus[ISP_IRQ_TYPE_DMA] =
		    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INT) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_DMA]));
		IrqStatus[ISP_IRQ_TYPE_INTB] =
		    (ISP_RD32((void *)ISP_REG_ADDR_INTB_STATUS) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_INTB]));
		IrqStatus[ISP_IRQ_TYPE_DMAB] =
		    (ISP_RD32((void *)ISP_REG_ADDR_DMAB_INT) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_DMAB]));
		IrqStatus[ISP_IRQ_TYPE_INTC] =
		    (ISP_RD32((void *)ISP_REG_ADDR_INTC_STATUS) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_INTC]));
		IrqStatus[ISP_IRQ_TYPE_DMAC] =
		    (ISP_RD32((void *)ISP_REG_ADDR_DMAC_INT) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_DMAC]));
		IrqStatus[ISP_IRQ_TYPE_INTX] =
		    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUSX) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_INTX]));
		IrqStatus[ISP_IRQ_TYPE_DMAX] =
		    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INTX) &
		     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] | g_IspInfo.IrqInfo.
		      ErrMask[ISP_IRQ_TYPE_DMAX]));

		for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
			if (g_IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i]) {
				/* LOG_ERR("Error IRQ, Type(%d), Status(0x%08X)",i,g_IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i]); */
				/* TODO: Add error handler... */
			}

			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG("Type(%d), IrqStatus(0x%08X | 0x%08X)", i,
					g_IspInfo.IrqInfo.Status[i], IrqStatus[i]);
				/* LOG_DBG("Mask(0x%08X), ErrMask(0x%08X)",g_IspInfo.IrqInfo.Mask[i], g_IspInfo.IrqInfo.ErrMask[i]); */
			}
			g_IspInfo.IrqInfo.Status[i] |= (IrqStatus[i] & g_IspInfo.IrqInfo.Mask[i]);
		}

		if ((g_IspInfo.IrqInfo.Status[WaitIrq.Type] & WaitIrq.Status) == WaitIrq.Status
		    || Timeout == 0) {
			break;
		}

		mdelay(1);
		Timeout -= 1;
	}

#else

	ISP_GetTime(&startTimeS, &startTimeUS);

	Timeout = wait_event_interruptible_timeout(g_IspInfo.WaitQueueHead,
						   ISP_GetIRQState(WaitIrq.Type, WaitIrq.Status),
						   ISP_MsToJiffies(WaitIrq.Timeout));
#endif

	if (Timeout == 0) {
		ISP_GetTime(&endTimeS, &endTimeUS);

		LOG_ERR("start(%u.%06u),end(%u.%06u)", startTimeS, startTimeUS, endTimeS,
			endTimeUS);
		LOG_ERR("Clear(%d),Type(%d),IrqStatus(0x%08X),WaitStatus(0x%08X),Timeout(%d)",
			WaitIrq.Clear, WaitIrq.Type, g_IspInfo.IrqInfo.Status[WaitIrq.Type],
			WaitIrq.Status, WaitIrq.Timeout);

		if (WaitIrq.Type == ISP_IRQ_TYPE_INT
		    && WaitIrq.Status == ISP_IRQ_INT_STATUS_AF_DON_ST) {
			/* Do nothing. */
		} else {
			ISP_DumpReg();
		}

		Ret = -EFAULT;
		goto EXIT;
	}

	spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);

	if (g_IspInfo.DebugMask & ISP_DBG_INT) {
		for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
			LOG_DBG("Type(%d), IrqStatus(0x%08X)", i, g_IspInfo.IrqInfo.Status[i]);
		}
	}

	g_IspInfo.IrqInfo.Status[WaitIrq.Type] &= (~WaitIrq.Status);

	spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);

	/* check CQ status, when pass2, pass2b, pass2c done */
	if (WaitIrq.Status == ISP_IRQ_INT_STATUS_PASS2_DON_ST ||
	    WaitIrq.Status == ISP_IRQ_INTB_STATUS_PASS2_DON_ST ||
	    WaitIrq.Status == ISP_IRQ_INTC_STATUS_PASS2_DON_ST) {
		MUINT32 CQ_status;

		if (ISP_RD32((void *)(CAMINF_BASE + 0x4160)) != ISP_REG_ADDR_CTL_DBG_SET_CQ_STS) {
			LOG_WRN("0x4160 != 0x6000 !!!(0x%8x)",
				ISP_RD32((void *)(CAMINF_BASE + 0x4160)));
			ISP_WR32((void *)(CAMINF_BASE + 0x4160), 0x6000);
		}

		CQ_status = ISP_RD32((void *)(CAMINF_BASE + 0x4164));

		switch (WaitIrq.Type) {
		case ISP_IRQ_TYPE_INT:
			if ((CQ_status & 0x0000000F) != 0x001) {
				LOG_ERR("CQ1 not idle dbg(0x%08x 0x%08x)",
					ISP_RD32((void *)(CAMINF_BASE + 0x4160)), CQ_status);
			}
			break;
		case ISP_IRQ_TYPE_INTB:
			if ((CQ_status & 0x000000F0) != 0x010) {
				LOG_ERR("CQ2 not idle dbg(0x%08x 0x%08x)",
					ISP_RD32((void *)(CAMINF_BASE + 0x4160)), CQ_status);
			}
			break;
		case ISP_IRQ_TYPE_INTC:
			if ((CQ_status & 0x00000F00) != 0x100) {
				LOG_ERR("CQ3 not idle dbg(0x%08x 0x%08x)",
					ISP_RD32((void *)(CAMINF_BASE + 0x4160)), CQ_status);
			}
			break;
		default:
			break;
		}
	}

EXIT:
	return Ret;
}

#define _debug_dma_err_
#if defined(_debug_dma_err_)

#define bit(x) (0x1<<(x))

MUINT32 DMA_ERR[3 * 12] = {
	bit(1), 0xF50043A8, 0x00000011,	/* IMGI */
	bit(2), 0xF50043AC, 0x00000021,	/* IMGCI */
	bit(4), 0xF50043B0, 0x00000031,	/* LSCI */
	bit(5), 0xF50043B4, 0x00000051,	/* FLKI */
	bit(6), 0xF50043B8, 0x00000061,	/* LCEI */
	bit(7), 0xF50043BC, 0x00000071,	/* VIPI */
	bit(8), 0xF50043C0, 0x00000081,	/* VIP2I */
	bit(9), 0xF50043C4, 0x00000194,	/* IMGO */
	bit(10), 0xF50043C8, 0x000001a4,	/* IMG2O */
	bit(11), 0xF50043CC, 0x000001b4,	/* LCSO */
	bit(12), 0xF50043D0, 0x000001c4,	/* ESFKO */
	bit(13), 0xF50043D4, 0x000001d4,	/* AAO */
};

static void DMAErrHandler(void)
{
	MUINT32 i = 0;
	MUINT32 err_ctrl = ISP_RD32(0xF50043A4);
	MUINT32 *pErr = DMA_ERR;

	LOG_DBG("err_ctrl(0x%08x)", err_ctrl);

	for (i = 0; i < 12; i++) {
		MUINT32 addr = 0;
#if 1
		if (err_ctrl & (*pErr)) {
			ISP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];
			LOG_DBG("(0x%08x,0x%08x),dbg(0x%08x, 0x%08x)", addr, ISP_RD32((unsigned long)addr),
				ISP_RD32(0xF5004160), ISP_RD32(0xF5004164));
		}
#else
		addr = pErr[1];
		MUINT32 status = ISP_RD32(addr);

		if (status & 0x0000FFFF) {
			ISP_WR32(0xF5004160, pErr[2]);
			addr = pErr[1];

			LOG_DBG("(0x%08x, 0x%08x), dbg(0x%08x, 0x%08x)",
				addr, status, ISP_RD32(0xF5004160), ISP_RD32(0xF5004164));
		}
#endif
		pErr = pErr + 3;
	}
}
#endif

/*******************************************************************************
*
********************************************************************************/
static __tcmfunc irqreturn_t ISP_Irq(MINT32 Irq, MVOID *DeviceId)
{
	/* LOG_DBG("- E."); */
	MUINT32 i;
	MUINT32 IrqStatus[ISP_IRQ_TYPE_AMOUNT];
	MUINT32 IrqStatus_fbc_int;
	MUINT32 IrqStatus_FBC;
	volatile MUINT32 tg_pa = 0;
	volatile MUINT32 sof_pa = 0;
	MUINT32 senifIntStA = 0, regVal = 0;
	CQ_RTBC_FBC imgo_fbc;
	CQ_RTBC_FBC img2o_fbc;
	/* //// */
	/* cam3 */
	MUINT32 j = 0, idx = 0, k = 0;
	eISPIrq eIrq = _IRQ;
	/* //// */

	/* ////// */
	/* CHRISTOPHER, cam3 */
	unsigned long long time_frmb_sec;
	unsigned long time_frmb_usec;

	time_frmb_sec = cpu_clock(0);	/* ns */
	do_div(time_frmb_sec, 1000);	/* usec */
	time_frmb_usec = do_div(time_frmb_sec, 1000000);	/* sec and usec */

	/* Read irq status */
	IrqStatus[ISP_IRQ_TYPE_INT] =
	    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUS) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_INT]));
	IrqStatus[ISP_IRQ_TYPE_DMA] =
	    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INT) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_DMA]));
	IrqStatus[ISP_IRQ_TYPE_INTB] =
	    (ISP_RD32((void *)ISP_REG_ADDR_INTB_STATUS) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_INTB]));
	IrqStatus[ISP_IRQ_TYPE_DMAB] =
	    (ISP_RD32((void *)ISP_REG_ADDR_DMAB_INT) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_DMAB]));
	IrqStatus[ISP_IRQ_TYPE_INTC] =
	    (ISP_RD32((void *)ISP_REG_ADDR_INTC_STATUS) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_INTC]));
	IrqStatus[ISP_IRQ_TYPE_DMAC] =
	    (ISP_RD32((void *)ISP_REG_ADDR_DMAC_INT) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_DMAC]));

	/* below may need to read elsewhere */
	IrqStatus[ISP_IRQ_TYPE_INTX] =
	    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUSX) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_INTX]));
	IrqStatus[ISP_IRQ_TYPE_DMAX] =
	    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INTX) &
	     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] | g_IspInfo.IrqInfo.
	      ErrMask[ISP_IRQ_TYPE_DMAX]));

	/* FBC drop frame status */
	IrqStatus_FBC = ISP_RD32((void *)ISP_REG_ADDR_FBC_INT);

	spin_lock(&SpinLockCamHaVer);
	if (!CAM_HAL_VER_IS3) {
		spin_unlock(&SpinLockCamHaVer);
		IrqStatus_fbc_int = ISP_RD32((void *)(ISP_ADDR + 0xFC));
		spin_lock(&(g_IspInfo.SpinLockIrq));

		for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
			if (g_IspInfo.IrqInfo.ErrMask[i] & IrqStatus[i]) {
				/* ISP_IRQ_INTX_STATUS_IMGO_ERR_ST: on-the-fly imgo error, not really overrun */
				if (i != ISP_IRQ_TYPE_INTX
				    || (g_IspInfo.IrqInfo.
					ErrMask[i] & IrqStatus[i] &
					(~ISP_IRQ_INTX_STATUS_IMGO_ERR_ST))) {
					if (i != ISP_IRQ_TYPE_INTX) {	/* to reduce isp log */
						LOG_DBG("Error IRQ, Type(%d), Status(0x%08X)", i,
							g_IspInfo.IrqInfo.
							ErrMask[i] & IrqStatus[i]);
					}
					/* TODO: Add error handler... */
				}
			}

			if (g_IspInfo.DebugMask & ISP_DBG_INT) {
				LOG_DBG("Type(%d), IrqStatus(0x%08X | 0x%08X)", i,
					g_IspInfo.IrqInfo.Status[i], IrqStatus[i]);
			}
			g_IspInfo.IrqInfo.Status[i] |= (IrqStatus[i] & g_IspInfo.IrqInfo.Mask[i]);
			/*  */
		}


#if defined(OVERRUN_AEE_WARNING)

		if (IrqStatus[ISP_IRQ_TYPE_INT] & ISP_IRQ_INT_STATUS_TG1_ERR_ST) {
			senifIntStA = ISP_RD32((void *)(ISP_ADDR + 0x4018));
			if (senifIntStA & 0x01) {
				regVal = ISP_RD32((void *)(ISP_ADDR + 0x414));
				if (regVal & 0x01) {
					LOG_DBG("sensor overrun,4024(0x%08x),8018(0x%08x)",
						IrqStatus[ISP_IRQ_TYPE_INT], senifIntStA);

					LOG_DBG("8014(0x%08x),8018(0x%08x)",
						ISP_RD32((void *)(ISP_ADDR + 0x4014)),
						ISP_RD32((void *)(ISP_ADDR + 0x4018)));
					LOG_DBG("4004(0x%08x),4008(0x%08x)",
						ISP_RD32((void *)(ISP_ADDR + 0x4)),
						ISP_RD32((void *)(ISP_ADDR + 0x8)));
					LOG_DBG("400C(0x%08x),4414(0x%08x)",
						ISP_RD32((void *)(ISP_ADDR + 0xC)),
						ISP_RD32((void *)(ISP_ADDR + 0x414)));
					LOG_DBG("4020(0x%08x),4024(0x%08x)",
						ISP_RD32((void *)(ISP_ADDR + 0x20)),
						ISP_RD32((void *)(ISP_ADDR + 0x24)));

					DMAErrHandler();
					SeninfOverrunDump();

					schedule_work(&g_IspInfo.ScheduleWorkSENINF);
					tasklet_schedule(&IspTaskletSENIF);
#if 0 /* TODO, temp remove, need to be added after aee is ok */
					aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT,
							       "CAMERA",
							       "SENIF FIFO overrun/underrun");
#endif
				}
			}
		}
#endif

		/* [MT]ToDo: Need to Check */
		/*if ( IrqStatus[ISP_IRQ_TYPE_INTX]&ISP_IRQ_INTX_STATUS_DMA_ERR_ST ) {

		   #if defined(_debug_dma_err_)
		   LOG_ERR("[rtbc]StatusX(0x%08X), DMA_ERR",IrqStatus[ISP_IRQ_TYPE_INTX]);  //IMGO may overrun before setting smi regs

		   //allan
		   DMAErrHandler();

		   LOG_DBG("ERR (8018 0x%08x)(8024 0x%08x)(802C 0x%08x)(4440 0x%08x) (810C 0x%08x)",
		   ISP_RD32(0xF5008018),
		   ISP_RD32(0xF5008024),
		   ISP_RD32(0xF500802C),
		   ISP_RD32(0xF5004440),
		   ISP_RD32(0xF500810C));
		   #endif

		   } */
		/*  */
		/* if ( IrqStatus[ISP_IRQ_TYPE_INT]&ISP_IRQ_INT_STATUS_TG1_ERR_ST || */
		/* IrqStatus[ISP_IRQ_TYPE_INT]&ISP_IRQ_INT_STATUS_TG2_ERR_ST ) { */
		/* LOG_ERR("[rtbc]Status(0x%08X), TG_ERR",IrqStatus[ISP_IRQ_TYPE_INT]); */
		/* } */
		/*  */
		/* if (IrqStatus_fbc_int) { */
		/* LOG_ERR("[rtbc]dropframe st(0x%08X),co(0x%08X),c2o(0x%08X)",IrqStatus_fbc_int,ISP_RD32(ISP_REG_ADDR_IMGO_FBC),ISP_RD32(ISP_REG_ADDR_IMG2O_FBC)); */
		/* } */

		spin_unlock(&(g_IspInfo.SpinLockIrq));

		/* service pass1_done first once if SOF/PASS1_DONE are coming together. */
		/* get time stamp */
		/* push hw filled buffer to sw list */
		if (IrqStatus[ISP_IRQ_TYPE_INT] & ISP_IRQ_INT_STATUS_PASS1_TG1_DON_ST) {
#if defined(_rtbc_use_cq0c_)

			if (g_IspInfo.DebugMask & ISP_DBG_BUF_CTRL) {
				LOG_DBG("[rtbc]:PASS1_TG1_DON");
			}

			/* set imgo exchange buffer address to HW */
			imgo_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC);
			tg_pa = ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR);

			if (g_newImgoAddr != DEFAULT_PA &&
			    (tg_pa == g_oldImgoAddr) &&
			    (imgo_fbc.Bits.FB_NUM != 0) &&
			    ((imgo_fbc.Bits.FB_NUM == imgo_fbc.Bits.FBC_CNT)
			     || (imgo_fbc.Bits.FBC_CNT == (imgo_fbc.Bits.FB_NUM - 1)))) {
				LOG_DBG("[rtbc][TG1_DON]o,tg_pa(0x%x),old(0x%x)", tg_pa,
					g_oldImgoAddr);
				LOG_DBG("[rtbc][TG1_DON]o,NUM(0x%x),CNT(0x%x)",
					imgo_fbc.Bits.FB_NUM, imgo_fbc.Bits.FBC_CNT);
				LOG_DBG("[rtbc][TG1_DON]o,new(0x%x)", g_newImgoAddr);
				ISP_WR32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR, g_newImgoAddr);
				g_oldImgoAddr = DEFAULT_PA;
				g_newImgoAddr = DEFAULT_PA;
			}

			img2o_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC);
			tg_pa = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR);

			if (g_newImg2oAddr != DEFAULT_PA &&
			    (tg_pa == g_oldImg2oAddr) &&
			    (img2o_fbc.Bits.FB_NUM != 0) &&
			    ((img2o_fbc.Bits.FB_NUM == img2o_fbc.Bits.FBC_CNT)
			     || (img2o_fbc.Bits.FBC_CNT == (img2o_fbc.Bits.FB_NUM - 1)))) {
				LOG_DBG("[rtbc][TG1_DON]2o,tg_pa(0x%x),old(0x%x)", tg_pa,
					g_oldImg2oAddr);
				LOG_DBG("[rtbc][TG1_DON]2o,NUM(0x%x),CNT(0x%x)",
					img2o_fbc.Bits.FB_NUM, img2o_fbc.Bits.FBC_CNT);
				LOG_DBG("[rtbc][TG1_DON]2o,new(0x%x)", g_newImg2oAddr);
				ISP_WR32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR, g_newImg2oAddr);
				g_oldImg2oAddr = DEFAULT_PA;
				g_newImg2oAddr = DEFAULT_PA;
			}
#else
			/* LOG_DBG("[k_js_test]Pass1_done(0x%x)",IrqStatus[ISP_IRQ_TYPE_INT]); */
			unsigned long long sec;
			unsigned long usec;

			sec = cpu_clock(0);	/* ns */
			do_div(sec, 1000);	/* usec */
			usec = do_div(sec, 1000000);	/* sec and usec */

			ISP_DONE_Buf_Time(sec, usec);
			/*Check Timesamp reverse */
			/* what's this? */
#endif

			g_sof_pass1done = 0;
		}

		/* switch pass1 WDMA buffer */
		/* fill time stamp for cq0c */
		if (IrqStatus[ISP_IRQ_TYPE_INT] & ISP_IRQ_INT_STATUS_SOF1_INT_ST) {
			unsigned long long sec;
			unsigned long usec;
			ktime_t time;
			MUINT32 imgo_pa, img2o_pa;
#if 1
			time = ktime_get();	/* ns */
			sec = time.tv64;
#else
			sec = cpu_clock(0);	/* ns */
#endif

			/* set imgo exchange buffer address to HW */
			imgo_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMGO_FBC);
			sof_pa = ISP_RD32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR);
			imgo_pa = sof_pa;

			if ((sof_pa != 0) && (sof_pa == g_oldImgoAddr)
			    && g_newImgoAddr != DEFAULT_PA && (imgo_fbc.Bits.FB_NUM != 0)
			    && ((imgo_fbc.Bits.FB_NUM == imgo_fbc.Bits.FBC_CNT)
				|| (imgo_fbc.Bits.FBC_CNT == (imgo_fbc.Bits.FB_NUM - 1)))) {
				LOG_DBG("[rtbc][SOF]o,sof_pa(0x%x),old(0x%x)", sof_pa,
					g_oldImgoAddr);
				LOG_DBG("[rtbc][SOF]o,NUM(0x%x),CNT(0x%x)", imgo_fbc.Bits.FB_NUM,
					imgo_fbc.Bits.FBC_CNT);
				LOG_DBG("[rtbc][SOF]o,new(0x%x)", g_newImgoAddr);

				ISP_WR32((void *)ISP_REG_ADDR_IMGO_BASE_ADDR, g_newImgoAddr);
				g_oldImgoAddr = DEFAULT_PA;
				g_newImgoAddr = DEFAULT_PA;
			}

			img2o_fbc.Reg_val = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_FBC);
			sof_pa = ISP_RD32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR);
			img2o_pa = sof_pa;

			if ((sof_pa != 0) && (sof_pa == g_oldImg2oAddr)
			    && g_newImg2oAddr != DEFAULT_PA && (img2o_fbc.Bits.FB_NUM != 0)
			    && ((img2o_fbc.Bits.FB_NUM == img2o_fbc.Bits.FBC_CNT)
				|| (img2o_fbc.Bits.FBC_CNT == (img2o_fbc.Bits.FB_NUM - 1)))) {
				LOG_DBG("[rtbc][SOF]2o,sof_pa(0x%x),old(0x%x)", sof_pa,
					g_oldImg2oAddr);
				LOG_DBG("[rtbc][SOF]2o,NUM(0x%x),CNT(0x%x)", img2o_fbc.Bits.FB_NUM,
					img2o_fbc.Bits.FBC_CNT);
				LOG_DBG("[rtbc][SOF]2o,new(0x%x/0x%x)", sof_pa, g_newImg2oAddr);

				ISP_WR32((void *)ISP_REG_ADDR_IMG2O_BASE_ADDR, g_newImg2oAddr);
				g_oldImg2oAddr = DEFAULT_PA;
				g_newImg2oAddr = DEFAULT_PA;
			}

			do_div(sec, 1000);	/* usec */
			usec = do_div(sec, 1000000);	/* sec and usec */

			ISP_SOF_Buf_Get(sec, usec);

			if (g_sof_pass1done == 1) {
				LOG_DBG("Lost pass1 done,P1_SOF_%d(0x%08lx,0x%08lx,0x%08x,0x%08x)",
				((ISP_RD32(ISP_REG_ADDR_TG_INTER_ST) & 0x00FF0000) >> 16),
				imgo_fbc.Reg_val, img2o_fbc.Reg_val, imgo_pa, img2o_pa);
			} else {
				LOG_DBG("P1_SOF_%d(0x%08lx,0x%08lx,0x%08x,0x%08x)",
								((ISP_RD32(ISP_REG_ADDR_TG_INTER_ST) &
								0x00FF0000) >> 16), imgo_fbc.Reg_val,
								img2o_fbc.Reg_val, imgo_pa, img2o_pa);
			}
			g_sof_pass1done = 1;
		}

		wake_up_interruptible(&g_IspInfo.WaitQueueHead);
	} else {
		spin_unlock(&SpinLockCamHaVer);
		/*  */
		ISP_Irq_FrmB(IrqStatus);
		/*  */
		spin_lock(&IspInfo_FrmB.SpinLockIrq[eIrq]);
		for (j = 0; j < ISP_IRQ_TYPE_AMOUNT_FRMB; j++) {
			for (i = 0; i < IRQ_USER_NUM_MAX; i++) {
				/* 1. update interrupt status to all users */
				IspInfo_FrmB.IrqInfo.Status[i][j] |=
				    (IrqStatus[j] & IspInfo_FrmB.IrqInfo.Mask[j]);

				/* 2. update signal occuring time and passed by signal count */
				if (IspInfo_FrmB.IrqInfo.MarkedFlag[i][j] & IspInfo_FrmB.IrqInfo.
				    Mask[j]) {
					for (k = 0; k < 32; k++) {
					/**/	if ((IrqStatus[j] & IspInfo_FrmB.IrqInfo.
						     Mask[j]) & (1 << k)) {
							idx = my_get_pow_idx(1 << k);
							IspInfo_FrmB.IrqInfo.
							    LastestSigTime_usec[j][idx] =
							    (unsigned int)(time_frmb_usec);
							IspInfo_FrmB.IrqInfo.
							    LastestSigTime_sec[j][idx] =
							    (unsigned int)(time_frmb_sec);
							IspInfo_FrmB.IrqInfo.
							    PassedBySigCnt[i][j][k]++;
						}
					}
				} else {	/* no any interrupt is not marked and  in read mask in this irq type */
				}
			}
		}
		spin_unlock(&IspInfo_FrmB.SpinLockIrq[eIrq]);
		/* 4. wake up users */
		wake_up_interruptible(&IspInfo_FrmB.WaitQueueHead);	/* could move to ISP_Irq_FrmB */
	}

	/* Work queue. It is interruptible, so there can be "Sleep" in work queue function. */
	if (IrqStatus[ISP_IRQ_TYPE_INT] & (ISP_IRQ_INT_STATUS_VS1_ST)) {	/* |ISP_IRQ_INT_STATUS_VS2_ST)) */
		g_IspInfo.TimeLog.Vd = ISP_JiffiesToMs(jiffies);
		schedule_work(&g_IspInfo.ScheduleWorkVD);
		tasklet_schedule(&IspTaskletVD);
	}

	/* Tasklet. It is uninterrupted, so there can NOT be "Sleep" in tasklet function. */
	if (IrqStatus[ISP_IRQ_TYPE_INT] & (ISP_IRQ_INT_STATUS_EXPDON1_ST)) {	/* |ISP_IRQ_INT_STATUS_EXPDON2_ST)) */
		g_IspInfo.TimeLog.Expdone = ISP_JiffiesToMs(jiffies);
		schedule_work(&g_IspInfo.ScheduleWorkEXPDONE);
		tasklet_schedule(&IspTaskletEXPDONE);
	}

	/* LOG_DBG("- X."); */

	return IRQ_HANDLED;
}

/*******************************************************************************
*
********************************************************************************/
static long ISP_ioctl(struct file *pFile, MUINT32 Cmd, unsigned long Param)
{
	MINT32 Ret = 0;
	MBOOL HoldEnable = MFALSE;
	unsigned long flags;
	MUINT32 DebugFlag[2] = { 0 }, pid = 0;
	ISP_REG_IO_STRUCT RegIo;
	ISP_HOLD_TIME_ENUM HoldTime;
	ISP_WAIT_IRQ_STRUCT WaitIrq;
	ISP_READ_IRQ_STRUCT ReadIrq;
	ISP_CLEAR_IRQ_STRUCT ClearIrq;
	ISP_USER_INFO_STRUCT *pUserInfo;
	MUINT32 wakelock_ctrl = 0;
	/* ////////////////////////////// */
	/* cam 3 */
	ISP_WAIT_IRQ_STRUCT_FRMB Irq_FrmB;
	ISP_ED_BUFQUE_STRUCT_FRMB edQueBuf_FrmB;
	int userKey = -1;
	/*MINT32 burstQNum;  unused, mark to fix build error */
	ISP_REGISTER_USERKEY_STRUCT_FRMB RegUserKey_FrmB;
	/* ////////////////////////////// */

	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		return -EFAULT;
	}

	pUserInfo = (ISP_USER_INFO_STRUCT *) (pFile->private_data);

	switch (Cmd) {
	case ISP_WAKELOCK_CTRL:
		{
			if (copy_from_user(&wakelock_ctrl, (void __user *)Param, sizeof(MUINT32)) !=
			    0) {
				LOG_ERR("get ISP_WAKELOCK_CTRL from user fail");
				Ret = -EFAULT;
			} else {
				if (wakelock_ctrl == 1) {	/* Enable wakelock */
					if (g_bWaitLock == 0) {
#ifdef CONFIG_PM_WAKELOCKS
						__pm_stay_awake(&isp_wake_lock);
#else
						wake_lock(&isp_wake_lock);
#endif
						g_bWaitLock = 1;
						LOG_DBG("wakelock enable!!\n");
					}
				} else {	/* Disable wakelock */

					if (g_bWaitLock == 1) {
#ifdef CONFIG_PM_WAKELOCKS
						__pm_relax(&isp_wake_lock);
#else
						wake_unlock(&isp_wake_lock);
#endif
						g_bWaitLock = 0;
						LOG_DBG("wakelock disable!!\n");
					}
				}
			}
			break;
		}
	case ISP_RESET:
		{
			spin_lock(&(g_IspInfo.SpinLockIsp));
			ISP_Reset();
			spin_unlock(&(g_IspInfo.SpinLockIsp));
			break;
		}
	case ISP_RESET_BUF:
		{
			spin_lock_bh(&(g_IspInfo.SpinLockHold));
			ISP_ResetBuf();
			spin_unlock_bh(&(g_IspInfo.SpinLockHold));
			break;
		}
	case ISP_READ_REGISTER:
		{
			if (copy_from_user(&RegIo, (void __user *)Param, sizeof(ISP_REG_IO_STRUCT))
			    == 0) {
				Ret = ISP_ReadReg(&RegIo);
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_WRITE_REGISTER:
		{
			if (copy_from_user(&RegIo, (void __user *)Param, sizeof(ISP_REG_IO_STRUCT))
			    == 0) {
				Ret = ISP_WriteReg(&RegIo);
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_HOLD_REG_TIME:
		{
			if (copy_from_user
			    (&HoldTime, (void __user *)Param, sizeof(ISP_HOLD_TIME_ENUM)) == 0) {
				spin_lock(&(g_IspInfo.SpinLockIsp));
				Ret = ISP_SetHoldTime(HoldTime);
				spin_unlock(&(g_IspInfo.SpinLockIsp));
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_HOLD_REG:
		{
			if (copy_from_user(&HoldEnable, (void __user *)Param, sizeof(MBOOL)) == 0) {
				Ret = ISP_EnableHoldReg(HoldEnable);
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_WAIT_IRQ:
		{
			if (copy_from_user
			    (&WaitIrq, (void __user *)Param, sizeof(ISP_WAIT_IRQ_STRUCT)) == 0) {
				if ((WaitIrq.Type >= ISP_IRQ_TYPE_AMOUNT) || (WaitIrq.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)", WaitIrq.Type);
					goto EXIT;
				}
				Ret = ISP_WaitIrq(WaitIrq);
			} else {
				LOG_ERR("copy_from_user 1  failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_READ_IRQ:
		{
			if (copy_from_user
			    (&ReadIrq, (void __user *)Param, sizeof(ISP_READ_IRQ_STRUCT)) == 0) {
				LOG_DBG("ISP_READ_IRQ Type(%d)", ReadIrq.Type);

				if ((ReadIrq.Type >= ISP_IRQ_TYPE_AMOUNT) || (ReadIrq.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)", ReadIrq.Type);
					goto EXIT;
				}
#if ISP_IRQ_POLLING

				spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);

				ISP_IRQ_TYPE_ENUM IrqStatus[ISP_IRQ_TYPE_AMOUNT];

				IrqStatus[ISP_IRQ_TYPE_INT] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUS) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INT]));
				IrqStatus[ISP_IRQ_TYPE_DMA] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INT) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMA]));
				IrqStatus[ISP_IRQ_TYPE_INTB] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INTB_STATUS) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INTB]));
				IrqStatus[ISP_IRQ_TYPE_DMAB] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMAB_INT) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMAB]));
				IrqStatus[ISP_IRQ_TYPE_INTC] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INTC_STATUS) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INTC]));
				IrqStatus[ISP_IRQ_TYPE_DMAC] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMAC_INT) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMAC]));
				IrqStatus[ISP_IRQ_TYPE_INTX] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUSX) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INTX]));
				IrqStatus[ISP_IRQ_TYPE_DMAX] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INTX) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMAX]));

				for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
					g_IspInfo.IrqInfo.Status[i] |=
					    (IrqStatus[i] & g_IspInfo.IrqInfo.Mask[i]);
				}

				spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);

#endif

				ReadIrq.Status = g_IspInfo.IrqInfo.Status[ReadIrq.Type];

				if (copy_to_user
				    ((void __user *)Param, &ReadIrq,
				     sizeof(ISP_READ_IRQ_STRUCT)) != 0) {
					LOG_ERR("copy_to_user failed");
					Ret = -EFAULT;
				}
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_CLEAR_IRQ:
		{
			if (copy_from_user
			    (&ClearIrq, (void __user *)Param, sizeof(ISP_CLEAR_IRQ_STRUCT)) == 0) {
				LOG_DBG("ISP_CLEAR_IRQ Type(%d)", ClearIrq.Type);

				if ((ClearIrq.Type >= ISP_IRQ_TYPE_AMOUNT) || (ClearIrq.Type < 0)) {
					Ret = -EFAULT;
					LOG_ERR("invalid type(%d)", ClearIrq.Type);
					goto EXIT;
				}
				spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);

#if ISP_IRQ_POLLING

				ISP_IRQ_TYPE_ENUM IrqStatus[ISP_IRQ_TYPE_AMOUNT];

				IrqStatus[ISP_IRQ_TYPE_INT] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUS) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INT]));
				IrqStatus[ISP_IRQ_TYPE_DMA] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INT) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMA]));
				IrqStatus[ISP_IRQ_TYPE_INTB] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INTB_STATUS) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INTB]));
				IrqStatus[ISP_IRQ_TYPE_DMAB] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMAB_INT) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMAB]));
				IrqStatus[ISP_IRQ_TYPE_INTC] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INTC_STATUS) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INTC]));
				IrqStatus[ISP_IRQ_TYPE_DMAC] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMAC_INT) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMAC]));
				IrqStatus[ISP_IRQ_TYPE_INTX] =
				    (ISP_RD32((void *)ISP_REG_ADDR_INT_STATUSX) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_INTX]));
				IrqStatus[ISP_IRQ_TYPE_DMAX] =
				    (ISP_RD32((void *)ISP_REG_ADDR_DMA_INTX) &
				     (g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] | g_IspInfo.IrqInfo.
				      ErrMask[ISP_IRQ_TYPE_DMAX]));

				for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
					g_IspInfo.IrqInfo.Status[i] |=
					    (IrqStatus[i] & g_IspInfo.IrqInfo.Mask[i]);
				}

#endif

				LOG_DBG("ISP_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)",
					ClearIrq.Type, ClearIrq.Status,
					g_IspInfo.IrqInfo.Status[ClearIrq.Type]);

				g_IspInfo.IrqInfo.Status[ClearIrq.Type] &= (~ClearIrq.Status);

				spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_DUMP_REG:
		{
			Ret = ISP_DumpReg();
			break;
		}
	case ISP_DEBUG_FLAG:
		{
			if (copy_from_user(&DebugFlag[0], (void __user *)Param, sizeof(MUINT32)) ==
			    0) {
				spin_lock_irqsave(&(g_IspInfo.SpinLockIrq), flags);
				g_IspInfo.DebugMask = DebugFlag[0];
				spin_unlock_irqrestore(&(g_IspInfo.SpinLockIrq), flags);
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_DEBUG_FLAG_FRMB:
		{
			if (copy_from_user(DebugFlag, (void __user *)Param, sizeof(MUINT32) * 2) ==
			    0) {
				MUINT32 lock_key = _IRQ_MAX;

				if (DebugFlag[1] >= _IRQ_MAX) {
					LOG_ERR("unsupported module:0x%x\n", DebugFlag[1]);
					Ret = -EFAULT;
					break;
				}
				if (DebugFlag[1] == _IRQ_D)
					lock_key = _IRQ;
				else
					lock_key = DebugFlag[1];
				spin_lock_irqsave(&(IspInfo_FrmB.SpinLockIrq[lock_key]), flags);
				IspInfo_FrmB.DebugMask = DebugFlag[0];
				spin_unlock_irqrestore(&(IspInfo_FrmB.SpinLockIrq[lock_key]),
						       flags);
				/* LOG_DBG("FBC kernel debug level = %x\n",IspInfo.DebugMask); */
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
	case ISP_SENSOR_FREQ_CTRL:
		{
			MUINT32 senFreq = 0;

			if (copy_from_user(&senFreq, (void __user *)Param, sizeof(MUINT32)) == 0) {
				LOG_DBG("senFreq=%u", senFreq);
				clkmux_sel(MT_MUX_CAMTG, senFreq, "CAMERA_SENSOR");
			} else {
				LOG_ERR("senFreq copy_from_user failed");
				Ret = -EFAULT;
			}
			break;
		}
#ifdef ISP_KERNEL_MOTIFY_SINGAL_TEST

	case ISP_SET_USER_PID:
		{
			if (copy_from_user(&pid, (void __user *)Param, sizeof(MUINT32)) == 0) {
				spin_lock(&(g_IspInfo.SpinLockIsp));
				getTaskInfo((pid_t) pid);

				sendSignal();

				LOG_DBG("[ISP_KERNEL_MOTIFY_SINGAL_TEST]:0x08%x ", pid);
				spin_unlock(&(g_IspInfo.SpinLockIsp));
			} else {
				LOG_ERR("copy_from_user failed");
				Ret = -EFAULT;
			}

			break;
		}
#endif
	case ISP_BUFFER_CTRL:
		spin_lock_irqsave(&SpinLockCamHaVer, flags);
		if (!CAM_HAL_VER_IS3) {
			spin_unlock_irqrestore(&SpinLockCamHaVer, flags);
			Ret = ISP_Buf_CTRL_FUNC(Param);
		} else {
			spin_unlock_irqrestore(&SpinLockCamHaVer, flags);
			Ret = ISP_Buf_CTRL_FUNC_FRMB(Param);
		}
		break;
	case ISP_REF_CNT_CTRL:
		Ret = ISP_REF_CNT_CTRL_FUNC(Param);
		break;
		/* /////////////////////////////////// */
		/* cam 3 */
	case ISP_SET_CAM_VERSION:
		{
			bool bIspVer;

			if (copy_from_user(&bIspVer, (void __user *)Param, sizeof(bool)) == 0) {
				spin_lock_irqsave(&SpinLockCamHaVer, flags);
				CAM_HAL_VER_IS3 = bIspVer;
				spin_unlock_irqrestore(&SpinLockCamHaVer, flags);
			}
			break;
		}
	case ISP_REGISTER_IRQ_FRMB:
		{
			break;
		}
	case ISP_WAIT_IRQ_FRMB:
		if (copy_from_user
		    (&Irq_FrmB, (void __user *)Param, sizeof(ISP_WAIT_IRQ_STRUCT_FRMB)) == 0) {
			if ((Irq_FrmB.UserInfo.UserKey >= IRQ_USER_NUM_MAX)
			    || (Irq_FrmB.UserInfo.UserKey <= 0)) {
				/* LOG_ERR("invalid userKey(%d), max(%d)",WaitIrq_FrmB.UserInfo.UserKey,IRQ_USER_NUM_MAX); */
				Irq_FrmB.UserInfo.UserKey = 0;
			}
			if ((Irq_FrmB.UserInfo.Type >= ISP_IRQ_TYPE_AMOUNT_FRMB)
			    || (Irq_FrmB.UserInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)", Irq_FrmB.UserInfo.Type,
					ISP_IRQ_TYPE_AMOUNT_FRMB);
				Ret = -EFAULT;
				break;
			}
			Ret = ISP_WaitIrq_FrmB(&Irq_FrmB);
			/*  */
			if (copy_to_user
			    ((void __user *)Param, &Irq_FrmB,
			     sizeof(ISP_WAIT_IRQ_STRUCT_FRMB)) != 0) {
				LOG_ERR("copy_to_user failed");
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("copy_from_user 3 failed");
			Ret = -EFAULT;
		}
		break;
	case ISP_ED_QUEBUF_CTRL_FRMB:
		if (copy_from_user
		    (&edQueBuf_FrmB, (void __user *)Param,
		     sizeof(ISP_ED_BUFQUE_STRUCT_FRMB)) == 0) {
			edQueBuf_FrmB.processID = pUserInfo->Pid;
			Ret = ISP_ED_BufQue_CTRL_FUNC_FRMB(edQueBuf_FrmB);
		} else {
			LOG_ERR("copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
	case ISP_UPDATE_BURSTQNUM_FRMB:
		{
			break;
		}
	case ISP_QUERY_BURSTQNUM_FRMB:
		{
			break;
		}
	case ISP_GET_CUR_SOF_FRMB:
		if (copy_from_user(&DebugFlag[0], (void __user *)Param, sizeof(MUINT32)) != 0) {
			LOG_ERR("get irq from user fail");
			Ret = -EFAULT;
		} else {
			switch (DebugFlag[0]) {
			case _IRQ:
				/* TG sof update at expo_done, causes potential timing issue */
				/* DebugFlag[1] = ((ISP_RD32(ISP_REG_ADDR_TG_INTER_ST)&0x00FF0000)>>16); */
				DebugFlag[1] = sof_count[0];
				break;
			default:
				LOG_ERR("err TG(0x%x)\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
		}
		if (copy_to_user((void __user *)Param, &DebugFlag[1], sizeof(MUINT32)) != 0) {
			LOG_ERR("copy to user fail");
			Ret = -EFAULT;
		}
		break;
	case ISP_GET_DMA_ERR_FRMB:
		if (copy_to_user((void __user *)Param, &g_DmaErr_p1[0], sizeof(MUINT32) * nDMA_ERR)
		    != 0) {
			LOG_ERR("get dma_err fail\n");
		}
		break;
	case ISP_GET_INT_ERR_FRMB:
		if (copy_to_user((void __user *)Param, g_ISPIntErr, sizeof(MUINT32) * _IRQ_MAX) !=
		    0) {
			LOG_ERR("get int err fail\n");
		}
		break;
	case ISP_SET_FPS_FRMB:
		{
			break;
		}
		/*  */
	case ISP_REGISTER_IRQ_USER_KEY:
		if (copy_from_user
		    (&RegUserKey_FrmB, (void __user *)Param,
		     sizeof(ISP_REGISTER_USERKEY_STRUCT_FRMB)) == 0) {
			userKey = ISP_REGISTER_IRQ_USERKEY(RegUserKey_FrmB.userName);
			RegUserKey_FrmB.userKey = userKey;
			if (copy_to_user
			    ((void __user *)Param, &RegUserKey_FrmB,
			     sizeof(ISP_REGISTER_USERKEY_STRUCT_FRMB)) != 0) {
				LOG_ERR("query irq user key fail\n");
			}
			if (RegUserKey_FrmB.userKey < 0) {
				Ret = -1;
			}
		}
		break;
		/*  */
	case ISP_MARK_IRQ_REQUEST:
		if (copy_from_user
		    (&Irq_FrmB, (void __user *)Param, sizeof(ISP_WAIT_IRQ_STRUCT_FRMB)) == 0) {
			if ((Irq_FrmB.UserInfo.UserKey >= IRQ_USER_NUM_MAX)
			    || (Irq_FrmB.UserInfo.UserKey < 1)) {
				LOG_ERR("invalid userKey(%d), max(%d)", Irq_FrmB.UserInfo.UserKey,
					IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((Irq_FrmB.UserInfo.Type >= ISP_IRQ_TYPE_AMOUNT_FRMB)
			    || (Irq_FrmB.UserInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)", Irq_FrmB.UserInfo.Type,
					ISP_IRQ_TYPE_AMOUNT_FRMB);
				Ret = -EFAULT;
				break;
			}
			Ret = ISP_MARK_IRQ(Irq_FrmB);
		} else {
			LOG_ERR("ISP_MARK_IRQ, copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
		/*  */
	case ISP_GET_MARK2QUERY_TIME:
		if (copy_from_user
		    (&Irq_FrmB, (void __user *)Param, sizeof(ISP_WAIT_IRQ_STRUCT_FRMB)) == 0) {
			if ((Irq_FrmB.UserInfo.UserKey >= IRQ_USER_NUM_MAX)
			    || (Irq_FrmB.UserInfo.UserKey < 1)) {
				LOG_ERR("invalid userKey(%d), max(%d)", Irq_FrmB.UserInfo.UserKey,
					IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((Irq_FrmB.UserInfo.Type >= ISP_IRQ_TYPE_AMOUNT_FRMB)
			    || (Irq_FrmB.UserInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)", Irq_FrmB.UserInfo.Type,
					ISP_IRQ_TYPE_AMOUNT_FRMB);
				Ret = -EFAULT;
				break;
			}
			Ret = ISP_GET_MARKtoQEURY_TIME(&Irq_FrmB);
			/*  */
			if (copy_to_user
			    ((void __user *)Param, &Irq_FrmB,
			     sizeof(ISP_WAIT_IRQ_STRUCT_FRMB)) != 0) {
				LOG_ERR("copy_to_user failed");
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("ISP_GET_MARK2QUERY_TIME, copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
		/*  */
	case ISP_FLUSH_IRQ_REQUEST:
		if (copy_from_user
		    (&Irq_FrmB, (void __user *)Param, sizeof(ISP_WAIT_IRQ_STRUCT_FRMB)) == 0) {
			if (!CAM_HAL_VER_IS3) {
				LOG_ERR("only support cam3");
				Ret = -EFAULT;
				break;
			}
			/* if((Irq_FrmB.UserInfo.UserKey>=IRQ_USER_NUM_MAX) || (Irq_FrmB.UserInfo.UserKey<1)) */
			/* UserKey(0) is Isp drv, flush ISP EnqueThread SOF signal */
			if (Irq_FrmB.UserInfo.UserKey != 0) {	/* isp driver */
				LOG_ERR("invalid userKey(%d), max(%d)", Irq_FrmB.UserInfo.UserKey,
					IRQ_USER_NUM_MAX);
				Ret = -EFAULT;
				break;
			}
			if ((Irq_FrmB.UserInfo.Type >= ISP_IRQ_TYPE_AMOUNT_FRMB)
			    || (Irq_FrmB.UserInfo.Type < 0)) {
				LOG_ERR("invalid type(%d), max(%d)", Irq_FrmB.UserInfo.Type,
					ISP_IRQ_TYPE_AMOUNT_FRMB);
				Ret = -EFAULT;
				break;
			}
			LOG_INF("User_%s(%d), type(%d)",
				IrqUserKey_UserInfo[Irq_FrmB.UserInfo.UserKey].userName,
				Irq_FrmB.UserInfo.UserKey, Irq_FrmB.UserInfo.Type);
			Ret = ISP_FLUSH_IRQ(Irq_FrmB);
		}
		break;
	case ISP_DUMP_ISR_LOG_FRMB:
		if (copy_from_user(DebugFlag, (void __user *)Param, sizeof(MUINT32)) == 0) {
			MUINT32 currentPPB = m_CurrentPPB;
			MUINT32 lock_key = _IRQ_MAX;

			if (DebugFlag[0] >= _IRQ_MAX) {
				LOG_ERR("unsupported module:0x%x\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
			if (DebugFlag[0] == _IRQ_D)
				lock_key = _IRQ;
			else
				lock_key = DebugFlag[0];
			spin_lock_irqsave(&(IspInfo_FrmB.SpinLockIrq[lock_key]), flags);
			m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
			spin_unlock_irqrestore(&(IspInfo_FrmB.SpinLockIrq[lock_key]), flags);

			IRQ_LOG_PRINTER(DebugFlag[0], currentPPB, _LOG_INF);
			IRQ_LOG_PRINTER(DebugFlag[0], currentPPB, _LOG_ERR);

		} else {
			LOG_ERR("copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
	case ISP_GET_DROP_FRAME_FRMB:
		if (copy_from_user(&DebugFlag[0], (void __user *)Param, sizeof(MUINT32)) != 0) {
			LOG_ERR("get irq from user fail");
			Ret = -EFAULT;
		} else {
			switch (DebugFlag[0]) {
			case _IRQ:
				spin_lock_irqsave(&(IspInfo_FrmB.SpinLockIrq[_IRQ]), flags);
				DebugFlag[1] = sof_pass1done[0];
				spin_unlock_irqrestore(&(IspInfo_FrmB.SpinLockIrq[_IRQ]), flags);
				break;
			default:
				LOG_ERR("err TG(0x%x)\n", DebugFlag[0]);
				Ret = -EFAULT;
				break;
			}
			if (copy_to_user((void __user *)Param, &DebugFlag[1], sizeof(MUINT32)) != 0) {
				LOG_ERR("copy to user fail");
				Ret = -EFAULT;
			}
		}
		break;
		/* /////////////////////////////////// */
	default:
		{
			LOG_ERR("Unknown Cmd(x%x)", Cmd);
			Ret = -EPERM;
			break;
		}
	}
EXIT:
	if (Ret != 0) {
		LOG_ERR("Fail, Cmd(%d), Pid(%d), (process, pid, tgid)=(%s, %d, %d)", Cmd,
			pUserInfo->Pid, current->comm, current->pid, current->tgid);
	}

	return Ret;
}

#ifdef CONFIG_COMPAT
static int compat_get_isp_read_register_data(compat_ISP_REG_IO_STRUCT __user *data32,
					     ISP_REG_IO_STRUCT __user *data)
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

static int compat_put_isp_read_register_data(compat_ISP_REG_IO_STRUCT __user *data32,
					     ISP_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	/* compat_uptr_t uptr; */
	int err = 0;
	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->pData); */
	/* err |= put_user(uptr, &data32->pData); */
	err |= get_user(count, &data->Count);
	err |= put_user(count, &data32->Count);
	return err;
}

static int compat_get_isp_buf_ctrl_frmb_struct_data(compat_ISP_BUFFER_CTRL_STRUCT_FRMB __user *
						    data32,
						    ISP_BUFFER_CTRL_STRUCT_FRMB __user *data)
{
	compat_uint_t tmp;
	compat_uptr_t uptr;
	int err = 0;

	err |= get_user(tmp, &data32->ctrl);
	err |= put_user(tmp, &data->ctrl);
	err |= get_user(tmp, &data32->buf_id);
	err |= put_user(tmp, &data->buf_id);

	err |= get_user(uptr, &data32->data_ptr);
	err |= put_user(uptr, (unsigned long __user *)&data->data_ptr);
	err |= get_user(uptr, &data32->ex_data_ptr);
	err |= put_user(uptr, (unsigned long __user *)&data->ex_data_ptr);
	err |= get_user(uptr, &data32->pExtend);
	err |= put_user(uptr, (unsigned long __user *)&data->pExtend);

	return err;
}

static int compat_put_isp_buf_ctrl_frmb_struct_data(compat_ISP_BUFFER_CTRL_STRUCT_FRMB __user *
						    data32,
						    ISP_BUFFER_CTRL_STRUCT_FRMB __user *data)
{
	compat_uint_t tmp;
	/* compat_uptr_t uptr; */
	int err= 0;

	err |= get_user(tmp, &data->ctrl);
	err |= put_user(tmp, &data32->ctrl);
	err |= get_user(tmp, &data->buf_id);
	err |= put_user(tmp, &data32->buf_id);

	/* Assume data pointer is unchanged. */

	return err;
}

static int compat_get_isp_buf_ctrl_struct_data(compat_ISP_BUFFER_CTRL_STRUCT __user *data32,
					       ISP_BUFFER_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp;
	compat_uptr_t uptr;
	int err = 0;

	err |= get_user(tmp, &data32->ctrl);
	err |= put_user(tmp, &data->ctrl);
	err |= get_user(tmp, &data32->buf_id);
	err |= put_user(tmp, &data->buf_id);

	err |= get_user(uptr, &data32->data_ptr);
	err |= put_user(uptr,  (unsigned long __user *)&data->data_ptr);
	err |= get_user(uptr, &data32->ex_data_ptr);
	err |= put_user(uptr,  (unsigned long __user *)&data->ex_data_ptr);
	err |= get_user(uptr, &data32->pExtend);
	err |= put_user(uptr,  (unsigned long __user *)&data->pExtend);

	return err;
}

static int compat_put_isp_buf_ctrl_struct_data(compat_ISP_BUFFER_CTRL_STRUCT __user *data32,
					       ISP_BUFFER_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp;
	/* compat_uptr_t uptr; */
	int err = 0;

	err |= get_user(tmp, &data->ctrl);
	err |= put_user(tmp, &data32->ctrl);
	err |= get_user(tmp, &data->buf_id);
	err |= put_user(tmp, &data32->buf_id);

	/* Assume data pointer is unchanged. */

	return err;
}

static int compat_get_isp_ref_cnt_ctrl_struct_data(compat_ISP_REF_CNT_CTRL_STRUCT __user *data32,
						   ISP_REF_CNT_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(tmp, &data32->ctrl);
	err |= put_user(tmp, &data->ctrl);
	err |= get_user(tmp, &data32->id);
	err |= put_user(tmp, &data->id);
	err |= get_user(uptr, &data32->data_ptr);
	err |= put_user(compat_ptr(uptr), &data->data_ptr);

	return err;
}

static int compat_put_isp_ref_cnt_ctrl_struct_data(compat_ISP_REF_CNT_CTRL_STRUCT __user *data32,
						   ISP_REF_CNT_CTRL_STRUCT __user *data)
{
	compat_uint_t tmp;
	/* compat_uptr_t uptr; */
	int err = 0;

	err = get_user(tmp, &data->ctrl);
	err |= put_user(tmp, &data32->ctrl);
	err |= get_user(tmp, &data->id);
	err |= put_user(tmp, &data32->id);
	/* Assume data pointer is unchanged. */
	/* err |= get_user(compat_ptr(uptr), &data->data_ptr); */
	/* err |= put_user(uptr, &data32->data_ptr); */

	return err;
}

static int compat_get_isp_register_irq_user_key_data(compat_ISP_REGISTER_USERKEY_STRUCT_FRMB __user
						     *data32,
						     ISP_REGISTER_USERKEY_STRUCT_FRMB __user *data)
{
	compat_uint_t tmp;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(tmp, &data32->userKey);
	err |= put_user(tmp, &data->userKey);
	err = get_user(uptr, &data32->userName);
	err |= put_user(uptr,  (unsigned long __user *)&data->userName);

	return err;
}

static int compat_put_isp_register_irq_user_key_data(compat_ISP_REGISTER_USERKEY_STRUCT_FRMB __user
						     *data32,
						     ISP_REGISTER_USERKEY_STRUCT_FRMB __user *data)
{
	compat_uint_t tmp;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(tmp, &data->userKey);
	err |= put_user(tmp, &data32->userKey);
	err = get_user(uptr,  (unsigned long __user *)&data->userName);
	err |= put_user(uptr, &data32->userName);

	return err;
}

#if 0
#define COMPAT_ISP_READ_REGISTER   _IOWR(ISP_MAGIC, ISP_CMD_READ_REG,      compat_ISP_REG_IO_STRUCT)
#define COMPAT_ISP_WRITE_REGISTER  _IOWR(ISP_MAGIC, ISP_CMD_WRITE_REG,     compat_ISP_REG_IO_STRUCT)
#define COMPAT_ISP_BUFFER_CTRL     _IOWR(ISP_MAGIC, ISP_CMD_RT_BUF_CTRL,    compat_ISP_BUFFER_CTRL_STRUCT)
#define COMPAT_ISP_REF_CNT_CTRL    _IOWR(ISP_MAGIC, ISP_CMD_REF_CNT,       compat_ISP_REF_CNT_CTRL_STRUCT)
#endif
static long ISP_ioctl_compat(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_ISP_READ_REGISTER:
		{
			compat_ISP_REG_IO_STRUCT __user *data32;
			ISP_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_isp_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_get_isp_read_register_data error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_READ_REGISTER,
						       (unsigned long)data);
			err = compat_put_isp_read_register_data(data32, data);
			if (err) {
				LOG_INF("compat_put_isp_read_register_data error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_ISP_WRITE_REGISTER:
		{
			compat_ISP_REG_IO_STRUCT __user *data32;
			ISP_REG_IO_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_isp_read_register_data(data32, data);
			if (err) {
				LOG_INF("COMPAT_ISP_WRITE_REGISTER error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_WRITE_REGISTER,
						       (unsigned long)data);
			return ret;
		}
	case COMPAT_ISP_BUFFER_CTRL:
		{
			if (CAM_HAL_VER_IS3) {
				compat_ISP_BUFFER_CTRL_STRUCT_FRMB __user *data32;
				ISP_BUFFER_CTRL_STRUCT_FRMB __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(*data));
				if (data == NULL)
					return -EFAULT;

				err = compat_get_isp_buf_ctrl_frmb_struct_data(data32, data);
				if (err) {
					LOG_INF("compat_get_isp_buf_ctrl_struct_data error!!!\n");
					return err;
				}
				ret =
				    filp->f_op->unlocked_ioctl(filp, ISP_BUFFER_CTRL,
							       (unsigned long)data);
				err = compat_put_isp_buf_ctrl_frmb_struct_data(data32, data);
				if (err) {
					LOG_INF("compat_put_isp_buf_ctrl_struct_data error!!!\n");
					return err;
				}
			} else {
				compat_ISP_BUFFER_CTRL_STRUCT __user *data32;
				ISP_BUFFER_CTRL_STRUCT __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(*data));
				if (data == NULL)
					return -EFAULT;

				err = compat_get_isp_buf_ctrl_struct_data(data32, data);
				if (err) {
					LOG_INF("compat_get_isp_buf_ctrl_struct_data error!!!\n");
					return err;
				}
				ret =
				    filp->f_op->unlocked_ioctl(filp, ISP_BUFFER_CTRL,
							       (unsigned long)data);
				err = compat_put_isp_buf_ctrl_struct_data(data32, data);
				if (err) {
					LOG_INF("compat_put_isp_buf_ctrl_struct_data error!!!\n");
					return err;
				}
			}
			return ret;
		}
	case COMPAT_ISP_REF_CNT_CTRL:
		{
			compat_ISP_REF_CNT_CTRL_STRUCT __user *data32;
			ISP_REF_CNT_CTRL_STRUCT __user *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_isp_ref_cnt_ctrl_struct_data(data32, data);
			if (err) {
				LOG_INF("compat_get_isp_ref_cnt_ctrl_struct_data error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_REF_CNT_CTRL, (unsigned long)data);
			err = compat_put_isp_ref_cnt_ctrl_struct_data(data32, data);
			if (err) {
				LOG_INF("compat_put_isp_ref_cnt_ctrl_struct_data error!!!\n");
				return err;
			}
			return ret;
		}
	case COMPAT_ISP_REGISTER_IRQ_USER_KEY:
		{
			compat_ISP_REGISTER_USERKEY_STRUCT_FRMB __user *data32;
			ISP_REGISTER_USERKEY_STRUCT_FRMB *data;
			int err;

			data32 = compat_ptr(arg);
			data = compat_alloc_user_space(sizeof(*data));
			if (data == NULL)
				return -EFAULT;

			err = compat_get_isp_register_irq_user_key_data(data32, data);
			if (err) {
				LOG_INF("compat_get_isp_register_irq_user_key_data error!!!\n");
				return err;
			}
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_REGISTER_IRQ_USER_KEY,
						       (unsigned long)data);
			err = compat_put_isp_register_irq_user_key_data(data32, data);
			if (err) {
				LOG_INF("compat_put_isp_register_irq_user_key_data error!!!\n");
				return err;
			}
			return ret;
		}
#ifdef ISP_KERNEL_MOTIFY_SINGAL_TEST
	case COMPAT_ISP_SET_USER_PID:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_SET_USER_PID,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
#endif
	case COMPAT_ISP_WAKELOCK_CTRL:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_WAKELOCK_CTRL,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
	case COMPAT_ISP_DEBUG_FLAG:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_DEBUG_FLAG,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
	case COMPAT_ISP_DEBUG_FLAG_FRMB:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_DEBUG_FLAG_FRMB,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
	case COMPAT_ISP_SENSOR_FREQ_CTRL:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_SENSOR_FREQ_CTRL,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
	case COMPAT_ISP_GET_CUR_SOF_FRMB:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_GET_CUR_SOF_FRMB,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
	case COMPAT_ISP_GET_INT_ERR_FRMB:
		{
			ret =
			    filp->f_op->unlocked_ioctl(filp, ISP_GET_INT_ERR_FRMB,
						       (unsigned long)compat_ptr(arg));
			return ret;
		}
	case ISP_RESET:	/* no arg */
	case ISP_RESET_BUF:	/* no arg */
	case ISP_HOLD_REG_TIME:	/* enum */
	case ISP_HOLD_REG:	/* scalar */
	case ISP_DUMP_REG:	/* no arg */
	case ISP_WAIT_IRQ:	/* struct fix */
	case ISP_READ_IRQ:	/* struct fix */
	case ISP_CLEAR_IRQ:	/* struct fix */

	case ISP_SENSOR_FREQ_CTRL:	/* scalar */
	case ISP_SET_CAM_VERSION:	/* scalar */
	case ISP_REGISTER_IRQ_FRMB:	/* empty */
	case ISP_WAIT_IRQ_FRMB:	/* struct ok */
	case ISP_ED_QUEBUF_CTRL_FRMB:	/* struct ok */
	case ISP_UPDATE_BURSTQNUM_FRMB:	/* empty */
	case ISP_QUERY_BURSTQNUM_FRMB:	/* empty */
	case ISP_GET_DMA_ERR_FRMB:	/* scalar */
	case ISP_SET_FPS_FRMB:	/* empty */

	case ISP_MARK_IRQ_REQUEST:	/* struct ok */
	case ISP_GET_MARK2QUERY_TIME:	/* struct ok */
	case ISP_FLUSH_IRQ_REQUEST:	/* struct ok */
	case ISP_DUMP_ISR_LOG_FRMB:	/* scalar */
	case ISP_GET_DROP_FRAME_FRMB:	/* scalar */
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
	}
}
#endif

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_open(struct inode *pInode, struct file *pFile)
{
	MINT32 Ret = 0;
	MUINT32 i;
	ISP_USER_INFO_STRUCT *pUserInfo;
	unsigned long flags;

	/* kernellog limit to (current+150) lines per second */
	pr_detect_count = get_detect_count();
	i = pr_detect_count + 250;
	set_detect_count(i);

	LOG_DBG("+,UserCount(%d)", g_IspInfo.UserCount);

	spin_lock(&(g_IspInfo.SpinLockIspRef));

	/* LOG_DBG("UserCount(%d)",g_IspInfo.UserCount); */

	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(ISP_USER_INFO_STRUCT), GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (process, pid, tgid)=(%s, %d, %d)", current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (ISP_USER_INFO_STRUCT *) pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}

	if (g_IspInfo.UserCount > 0) {
		g_IspInfo.UserCount++;
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			g_IspInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	}

	#if (LOG_CONSTRAINT_ADJ == 1)
	g_log_def_constraint = detect_count;
	detect_count = 200;
	#endif

	/* default cam 1 */
	spin_lock_irqsave(&SpinLockCamHaVer, flags);
	CAM_HAL_VER_IS3 = false;
	spin_unlock_irqrestore(&SpinLockCamHaVer, flags);

	g_IspInfo.BufInfo.Read.pData = (MUINT8 *) kmalloc(ISP_BUF_SIZE, GFP_ATOMIC);
	g_IspInfo.BufInfo.Read.Size = ISP_BUF_SIZE;
	g_IspInfo.BufInfo.Read.Status = ISP_BUF_STATUS_EMPTY;
	if (g_IspInfo.BufInfo.Read.pData == NULL) {
		LOG_DBG("ERROR: BufRead kmalloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}

	if (!ISP_BufWrite_Alloc()) {
		LOG_DBG("ERROR: BufWrite kmalloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}

	atomic_set(&(g_IspInfo.HoldInfo.HoldEnable), 0);
	atomic_set(&(g_IspInfo.HoldInfo.WriteEnable), 0);
	for (i = 0; i < ISP_REF_CNT_ID_MAX; i++) {
		atomic_set(&g_imem_ref_cnt[i], 0);
	}

	/* cam 3 variable initilization */
	ISP_open_FrmB();
	/*  */

	/* Enable clock */
	ISP_EnableClock(MTRUE, 1);

	for (i = 0; i < ISP_IRQ_TYPE_AMOUNT; i++) {
		g_IspInfo.IrqInfo.Status[i] = 0;
	}

	for (i = 0; i < ISP_CALLBACK_AMOUNT; i++) {
		g_IspInfo.Callback[i].Func = NULL;
	}

	g_IspInfo.UserCount++;

	LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), first user",
		g_IspInfo.UserCount, current->comm, current->pid, current->tgid);

/* js_test */
/* g_IspInfo.DebugMask = ISP_DBG_BUF_CTRL; */

EXIT:

	if (Ret < 0) {
		if (g_IspInfo.BufInfo.Read.pData != NULL) {
			kfree(g_IspInfo.BufInfo.Read.pData);
			g_IspInfo.BufInfo.Read.pData = NULL;
		}

		ISP_BufWrite_Free();
	}

	spin_unlock(&(g_IspInfo.SpinLockIspRef));

	/* LOG_DBG("Before spm_disable_sodi()."); */
	/* Disable sodi (Multi-Core Deep Idle). */
	/* spm_disable_sodi(); */

	LOG_DBG("-,Ret(%d),UserCount(%d)", Ret, g_IspInfo.UserCount);

	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_release(struct inode *pInode, struct file *pFile)
{
	ISP_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("+,UserCount(%d)", g_IspInfo.UserCount);

	spin_lock(&(g_IspInfo.SpinLockIspRef));

	/* LOG_DBG("UserCount(%d)",g_IspInfo.UserCount); */

	if (pFile->private_data != NULL) {
		pUserInfo = (ISP_USER_INFO_STRUCT *) pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}

	g_IspInfo.UserCount--;
	if (g_IspInfo.UserCount > 0) {
		LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), users exist",
			g_IspInfo.UserCount, current->comm, current->pid, current->tgid);
		goto EXIT;
	}

	#if (LOG_CONSTRAINT_ADJ == 1)
	detect_count = g_log_def_constraint;
	#endif

	LOG_DBG("Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d), last user",
		g_IspInfo.UserCount, current->comm, current->pid, current->tgid);

	/* Disable clock. */
	ISP_EnableClock(MFALSE, 1);
	if (g_bWaitLock == 1) {
#ifdef CONFIG_PM_WAKELOCKS
		__pm_relax(&isp_wake_lock);
#else
		wake_unlock(&isp_wake_lock);
#endif
		g_bWaitLock = 0;
	}

	if (g_IspInfo.BufInfo.Read.pData != NULL) {
		kfree(g_IspInfo.BufInfo.Read.pData);
		g_IspInfo.BufInfo.Read.pData = NULL;
		g_IspInfo.BufInfo.Read.Size = 0;
		g_IspInfo.BufInfo.Read.Status = ISP_BUF_STATUS_EMPTY;
	}

	ISP_BufWrite_Free();

	/* cam 3 variable un-initilization */
	ISP_release_FrmB();
	/*  */

	mMclk1User = 0;
	ISP_WR32(ISP_ADDR + 0x4200, 0x00000001);
	/*LOG_DBG("ISP_MCLK1_EN release\n");*/

	/* kernel log limit back to default */
	set_detect_count(pr_detect_count);

EXIT:

	spin_unlock(&(g_IspInfo.SpinLockIspRef));

	/* LOG_DBG("Before spm_enable_sodi()."); */
	/* Enable sodi (Multi-Core Deep Idle). */
	/* spm_enable_sodi(); */

	LOG_DBG("-,UserCount(%d)", g_IspInfo.UserCount);
	return 0;
}

/*******************************************************************************
* helper function, mmap's the kmalloc'd area which is physically contiguous
********************************************************************************/
static MINT32 mmap_kmem(struct file *filp, struct vm_area_struct *vma)
{
	MINT32 ret;
	unsigned long length = 0;

	length = (vma->vm_end - vma->vm_start);

	/* check length - do not allow larger mappings than the number of  pages allocated */
	if (length > RT_BUF_TBL_NPAGES * PAGE_SIZE) {
		return -EIO;
	}

	/* map the whole physically contiguous area in one piece */
	LOG_INF("Vma->vm_pgoff(0x%x),Vma->vm_start(0x%x),Vma->vm_end(0x%x),length(0x%x)",
		(unsigned int)(vma->vm_pgoff), (unsigned int)(vma->vm_start),
		(unsigned int)(vma->vm_end), (unsigned int)(length));
	if (length > ISP_RTBUF_REG_RANGE) {
		LOG_ERR("mmap range error! : length(0x%x),ISP_RTBUF_REG_RANGE(0x%x)!",
			(unsigned int)(length), ISP_RTBUF_REG_RANGE);
		return -EAGAIN;
	}
	ret = remap_pfn_range(vma,
				   vma->vm_start,
				   virt_to_phys((void *)g_pTbl_RTBuf) >> PAGE_SHIFT,
				   length, vma->vm_page_prot);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	/* LOG_DBG("+"); */
	unsigned long length = 0;
	MUINT32 pfn = 0x0;

	length = (pVma->vm_end - pVma->vm_start);
	/* at offset RT_BUF_TBL_NPAGES we map the kmalloc'd area */
	if (pVma->vm_pgoff == RT_BUF_TBL_NPAGES) {
		return mmap_kmem(pFile, pVma);
	} else {
		pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
		LOG_INF
		    ("pVma->vm_pgoff(0x%x),phy(0x%x),pVmapVma->vm_start(0x%x),pVma->vm_end(0x%x),length(0x%x)",
		     (unsigned int)(pVma->vm_pgoff), (unsigned int)(pVma->vm_pgoff << PAGE_SHIFT),
		     (unsigned int)(pVma->vm_start), (unsigned int)(pVma->vm_end),
		     (unsigned int)(length));
		pfn = pVma->vm_pgoff << PAGE_SHIFT;
		switch (pfn) {
		case IMGSYS_BASE_ADDR:	/* imgsys */
			if (length > ISP_REG_RANGE) {
				LOG_ERR("mmap range error : length(0x%x),ISP_REG_RANGE(0x%x)!",
					(unsigned int)(length), ISP_REG_RANGE);
				return -EAGAIN;
			}
			break;
		case SENINF_BASE_ADDR:
			if (length > SENINF_REG_RANGE) {
				LOG_ERR("mmap range error : length(0x%x),SENINF_REG_RANGE(0x%x)!",
					(unsigned int)(length), SENINF_REG_RANGE);
				return -EAGAIN;
			}
			break;
		case PLL_BASE_ADDR:
			if (length > PLL_RANGE) {
				LOG_ERR("mmap range error : length(0x%x),PLL_RANGE(0x%x)!",
					(unsigned int)(length), PLL_RANGE);
				return -EAGAIN;
			}
			break;
		case MIPIRX_CONFIG_ADDR:
			if (length > MIPIRX_CONFIG_RANGE) {
				LOG_ERR
				    ("mmap range error : length(0x%x),MIPIRX_CONFIG_RANGE(0x%x)!",
				     (unsigned int)(length), MIPIRX_CONFIG_RANGE);
				return -EAGAIN;
			}
			break;
		case MIPIRX_ANALOG_ADDR:
			if (length > MIPIRX_ANALOG_RANGE) {
				LOG_ERR
				    ("mmap range error : length(0x%x),MIPIRX_ANALOG_RANGE(0x%x)!",
				     (unsigned int)(length), MIPIRX_ANALOG_RANGE);
				return -EAGAIN;
			}
			break;
		case GPIO_BASE_ADDR:
			if (length > GPIO_RANGE) {
				LOG_ERR("mmap range error : length(0x%x),GPIO_RANGE(0x%x)!",
					(unsigned int)(length), GPIO_RANGE);
				return -EAGAIN;
			}
			break;
		case EFUSE_BASE_ADDR:
			if (length > EFUSE_RANGE) {
				LOG_ERR("mmap range error : length(0x%x),EFUSE_RANGE(0x%x)!",
					(unsigned int)(length), EFUSE_RANGE);
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
	}

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
#ifdef CONFIG_OF
struct cam_isp_device {
	void __iomem *regs[ISP_CAM_BASEADDR_NUM];
	struct device *dev;
	int irq[ISP_CAM_IRQ_IDX_NUM];
};

static struct cam_isp_device *cam_isp_devs;
static int nr_camisp_devs;
#endif


static const struct file_operations g_IspFileOper = {

	.owner = THIS_MODULE,
	.open = ISP_open,
	.release = ISP_release,
	/* .flush   = mt_isp_flush, */
	.mmap = ISP_mmap,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ISP_ioctl_compat,
#endif
	.unlocked_ioctl = ISP_ioctl
};

/*******************************************************************************
*
********************************************************************************/
static inline MVOID ISP_UnregCharDev(MVOID)
{
	LOG_DBG("+");

	/* Release char driver */
	if (g_pIspCharDrv != NULL) {
		cdev_del(g_pIspCharDrv);
		g_pIspCharDrv = NULL;
	}

	unregister_chrdev_region(g_IspDevNo, 1);
}

/*******************************************************************************
*
********************************************************************************/
static inline MINT32 ISP_RegCharDev(MVOID)
{
	MINT32 Ret = 0;

	/*LOG_DBG("+");*/

	Ret = alloc_chrdev_region(&g_IspDevNo, 0, 1, ISP_DEV_NAME);
	if ((Ret) < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d", Ret);
		return Ret;
	}

	/* Allocate driver */
	g_pIspCharDrv = cdev_alloc();
	if (g_pIspCharDrv == NULL) {
		LOG_ERR("cdev_alloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}

	/* Attatch file operation. */
	cdev_init(g_pIspCharDrv, &g_IspFileOper);

	g_pIspCharDrv->owner = THIS_MODULE;

	/* Add to system */
	Ret = cdev_add(g_pIspCharDrv, g_IspDevNo, 1);
	if ((Ret) < 0) {
		LOG_ERR("Attatch file operation failed, %d", Ret);
		goto EXIT;
	}

EXIT:

	if (Ret < 0) {
		ISP_UnregCharDev();
	}

	/*LOG_DBG("-");*/
	return Ret;
}

/* //////////////////////////////////////////////////////////////////////////////////////// */
/* cam 3 */
/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_probe_FrmB(void)
{
	MINT32 Ret = 0;
	/*struct resource *pRes = NULL;*/
	/*MINT32 i = 0;*/
	MUINT8 n;
	/*  */
	init_waitqueue_head(&IspInfo_FrmB.WaitQueueHead);
	/*  */
	/* INIT_WORK(&IspInfo_FrmB.ScheduleWorkVD,       ISP_ScheduleWork_VD); */
	/* INIT_WORK(&IspInfo_FrmB.ScheduleWorkEXPDONE,  ISP_ScheduleWork_EXPDONE); */
	/*  */
	/* spin_lock_init(&(IspInfo_FrmB.SpinLockIspRef)); */
	/* spin_lock_init(&(IspInfo_FrmB.SpinLockIsp)); */
	for (n = 0; n < _IRQ_MAX; n++)
		spin_lock_init(&(IspInfo_FrmB.SpinLockIrq[n]));
	/* spin_lock_init(&(IspInfo_FrmB.SpinLockHold)); */
	spin_lock_init(&(IspInfo_FrmB.SpinLockRTBC));
	/* spin_lock_init(&(IspInfo_FrmB.SpinLockClock)); */
	/*  */
	/* IspInfo_FrmB.UserCount = 0; */
	/* IspInfo_FrmB.HoldInfo.Time = ISP_HOLD_TIME_EXPDONE; */
	/*  */
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_INT] = ISP_REG_MASK_INT_STATUS;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] = ISP_REG_MASK_DMA_INT;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] = ISP_REG_MASK_INTB_STATUS;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] = ISP_REG_MASK_DMAB_INT;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] = ISP_REG_MASK_INTC_STATUS;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] = ISP_REG_MASK_DMAC_INT;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] = ISP_REG_MASK_INTX_STATUS;
	IspInfo_FrmB.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] = ISP_REG_MASK_DMAX_INT;

	IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT] = ISP_REG_MASK_INT_STATUS_ERR;
	IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA] = ISP_REG_MASK_DMA_INT_ERR;
	IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB] = ISP_REG_MASK_INTB_STATUS_ERR;
	/* IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB] = ISP_REG_MASK_DMAB_INT_ERR; */
	IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC] = ISP_REG_MASK_INTC_STATUS_ERR;
	/* IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC] = ISP_REG_MASK_DMAC_INT_ERR; */
	IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX] = ISP_REG_MASK_INTX_STATUS_ERR_FRMB;
	IspInfo_FrmB.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX] = ISP_REG_MASK_DMAX_INT_ERR;

	/* enqueue/dequeue control in ihalpipe wrapper */
	init_waitqueue_head(&WaitQueueHead_EDBuf_WaitDeque);
	init_waitqueue_head(&WaitQueueHead_EDBuf_WaitFrame);
	spin_lock_init(&(SpinLockEDBufQueList));
	spin_lock_init(&(SpinLockRegScen));
	/*  */
	/* irq related */
	spin_lock_init(&(SpinLock_UserKey));
	return Ret;
}


/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static MINT32 ISP_remove_FrmB(void)
{
#if 0
	struct resource *pRes;
	MINT32 i;
	MINT32 IrqNum;
#endif
	/*  */
	return 0;
}

/* //////////////////////////////////////////////////////////////////////////////////////// */


/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_probe(struct platform_device *pDev)
{
	MINT32 Ret = 0;
	struct resource *pRes = NULL;
	MINT32 i;
#ifdef CONFIG_OF
	int new_count;
	struct cam_isp_device *cam_isp_dev;
#endif

	LOG_INF("isp Probe +");

	/* Check platform_device parameters */
	if (pDev == NULL) {
		dev_err(&pDev->dev, "pDev is NULL");
		return -ENXIO;
	}
#ifdef CONFIG_OF
	new_count = nr_camisp_devs + 1;
	cam_isp_devs = krealloc(cam_isp_devs,
				sizeof(struct cam_isp_device) * new_count, GFP_KERNEL);
	if (!cam_isp_devs) {
		dev_err(&pDev->dev, "Unable to allocate cam_isp_devs\n");
		return -ENOMEM;
	}
	cam_isp_dev = &(cam_isp_devs[nr_camisp_devs]);
	cam_isp_dev->dev = &pDev->dev;
	/* iomap registers and irq */
	for (i = 0; i < ISP_CAM_BASEADDR_NUM; i++) {
		cam_isp_dev->regs[i] = of_iomap(pDev->dev.of_node, i);
		if (!cam_isp_dev->regs[i]) {
			dev_err(&pDev->dev, "Unable to ioremap registers, of_iomap fail, i=%d\n",
				i);
			return -ENOMEM;
		}

		gISPSYS_Reg[i] = (unsigned long)(cam_isp_dev->regs[i]);
		gISPSYS_Reg_FrmB[i] = (unsigned long)(cam_isp_dev->regs[i]);
		LOG_INF("DT, i=%d, name=%s, map_addr=0x%lx/0x%lx\n", i, pDev->dev.of_node->name,
			gISPSYS_Reg[i], gISPSYS_Reg_FrmB[i]);
	}

	/* get IRQ ID and request IRQ */
	for (i = 0; i < ISP_CAM_IRQ_IDX_NUM; i++) {
		cam_isp_dev->irq[i] = irq_of_parse_and_map(pDev->dev.of_node, i);
		gISPSYS_Irq[i] = cam_isp_dev->irq[i];
		gISPSYS_Irq_FrmB[i] = cam_isp_dev->irq[i];
		if (ISP_CAM0_IRQ_IDX == i) {
			Ret = request_irq(cam_isp_dev->irq[i], (irq_handler_t) ISP_Irq, IRQF_TRIGGER_NONE, "ISP", NULL);	/* IRQF_TRIGGER_NONE dose not take effect here, real trigger mode set in dts file */
		}
		if (Ret) {
			dev_err(&pDev->dev,
				"Unable to request IRQ, request_irq fail, i=%d, irq=%d\n", i,
				cam_isp_dev->irq[i]);
			return Ret;
		}
		LOG_INF("DT, i=%d, name=%s, map_irq=%d\n", i, pDev->dev.of_node->name,
			cam_isp_dev->irq[i]);

	}
	nr_camisp_devs = new_count;
#endif



	/* Register char driver */
	Ret = ISP_RegCharDev();
	if ((Ret)) {
		dev_err(&pDev->dev, "register char failed");
		return Ret;
	}

	/* Mapping CAM_REGISTERS */
	for (i = 0; i < 1; i++) {	/* NEED_TUNING_BY_CHIP. 1: Only one IORESOURCE_MEM type resource in kernel\mt_devs.c\mt_resource_isp[]. */
		LOG_DBG("Mapping CAM_REGISTERS. i: %d.", i);

		pRes = platform_get_resource(pDev, IORESOURCE_MEM, i);
		if (pRes == NULL) {
			dev_err(&pDev->dev, "platform_get_resource failed");
			Ret = -ENOMEM;
			goto EXIT;
		}

		pRes = request_mem_region(pRes->start, pRes->end - pRes->start + 1, pDev->name);
		if (pRes == NULL) {
			dev_err(&pDev->dev, "request_mem_region failed");
			Ret = -ENOMEM;
			goto EXIT;
		}
	}

	/* Create class register */
	g_pIspClass = class_create(THIS_MODULE, "ispdrv");
	if (IS_ERR(g_pIspClass)) {
		Ret = PTR_ERR(g_pIspClass);
		LOG_ERR("Unable to create class, err = %d", Ret);
		return Ret;
	}

	/* FIXME: error handling */
	device_create(g_pIspClass, NULL, g_IspDevNo, NULL, ISP_DEV_NAME);

	init_waitqueue_head(&g_IspInfo.WaitQueueHead);
#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&isp_wake_lock, "isp_lock_wakelock");
#else
	wake_lock_init(&isp_wake_lock, WAKE_LOCK_SUSPEND, "isp_lock_wakelock");
#endif

	INIT_WORK(&g_IspInfo.ScheduleWorkVD, ISP_ScheduleWork_VD);
	INIT_WORK(&g_IspInfo.ScheduleWorkEXPDONE, ISP_ScheduleWork_EXPDONE);
	INIT_WORK(&g_IspInfo.ScheduleWorkSENINF, ISP_ScheduleWork_SENINF);

	spin_lock_init(&(g_IspInfo.SpinLockIspRef));
	spin_lock_init(&(g_IspInfo.SpinLockIsp));
	spin_lock_init(&(g_IspInfo.SpinLockIrq));
	spin_lock_init(&(g_IspInfo.SpinLockHold));
	spin_lock_init(&(g_IspInfo.SpinLockRTBC));
	spin_lock_init(&SpinLockCamHaVer);

	g_IspInfo.UserCount = 0;
	g_IspInfo.HoldInfo.Time = ISP_HOLD_TIME_EXPDONE;

	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INT] = ISP_REG_MASK_INT_STATUS;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMA] = ISP_REG_MASK_DMA_INT;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTB] = ISP_REG_MASK_INTB_STATUS;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAB] = ISP_REG_MASK_DMAB_INT;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTC] = ISP_REG_MASK_INTC_STATUS;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAC] = ISP_REG_MASK_DMAC_INT;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_INTX] = ISP_REG_MASK_INTX_STATUS;
	g_IspInfo.IrqInfo.Mask[ISP_IRQ_TYPE_DMAX] = ISP_REG_MASK_DMAX_INT;

	g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INT] = ISP_REG_MASK_INT_STATUS_ERR;
	g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMA] = ISP_REG_MASK_DMA_INT_ERR;
	g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTB] = ISP_REG_MASK_INTB_STATUS_ERR;
	/* g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAB] = ISP_REG_MASK_DMAB_INT_ERR; */
	g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTC] = ISP_REG_MASK_INTC_STATUS_ERR;
	/* g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAC] = ISP_REG_MASK_DMAC_INT_ERR; */
	g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_INTX] = ISP_REG_MASK_INTX_STATUS_ERR;
	g_IspInfo.IrqInfo.ErrMask[ISP_IRQ_TYPE_DMAX] = ISP_REG_MASK_DMAX_INT_ERR;

	/* cam 3 */
	ISP_probe_FrmB();
	/*  */

	/* Request CAM_ISP IRQ */
#ifndef CONFIG_OF
	if (request_irq(CAMERA_ISP_IRQ0_ID, (irq_handler_t) ISP_Irq, IRQF_TRIGGER_LOW, "isp", NULL))
/* if (request_irq(CAMERA_ISP_IRQ0_ID, (irq_handler_t)ISP_Irq, IRQF_TRIGGER_HIGH, "isp", NULL)) */
	{
		LOG_ERR("MT_CAM_IRQ_LINE IRQ LINE NOT AVAILABLE!!");
		goto EXIT;
	}
	/* mt_irq_unmask(CAMERA_ISP_IRQ0_ID); */
#endif
	/*  */

/* #endif */

EXIT:

	if (Ret < 0) {
		ISP_UnregCharDev();
	}

	LOG_DBG("isp probe-");
	return Ret;
}

/*******************************************************************************
* Called when the device is being detached from the driver
********************************************************************************/
static MINT32 ISP_remove(struct platform_device *pDev)
{
	struct resource *pRes;
	MINT32 i;
	MINT32 IrqNum;
	MINT32 Ret = 0;

	LOG_DBG("+");

	/* unregister char driver. */
	ISP_UnregCharDev();

	/* unmaping ISP CAM_REGISTER registers */
	for (i = 0; i < 2; i++) {
		pRes = platform_get_resource(pDev, IORESOURCE_MEM, i);
		if (pRes == NULL) {
			dev_err(&pDev->dev, "platform_get_resource failed");
			Ret = -ENOMEM;
		} else
			release_mem_region(pRes->start, (pRes->end - pRes->start + 1));

	}

	/* Release IRQ */
	disable_irq(g_IspInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* cam 3 */
	ISP_remove_FrmB();
	/*  */

	device_destroy(g_pIspClass, g_IspDevNo);

	class_destroy(g_pIspClass);
	g_pIspClass = NULL;
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_suspend(struct platform_device *pDev, pm_message_t Mesg)
{
	/* TG_VF_CON[0] (0x15004414[0]): VFDATA_EN. TG1 Take Picture Request. */
	MUINT32 regTG1Val = ISP_RD32((void *)(ISP_ADDR + 0x414));

	LOG_DBG("g_bPass1_On_In_Resume_TG1(%d),regTG1Val(0x%08x)", g_bPass1_On_In_Resume_TG1,
		regTG1Val);

	g_bPass1_On_In_Resume_TG1 = 0;
	if (regTG1Val & 0x01) {	/* For TG1 Main sensor. */
		g_bPass1_On_In_Resume_TG1 = 1;
		ISP_WR32((void *)(ISP_ADDR + 0x414), (regTG1Val & (~0x01)));
		/* ISP_Reset(); */
	}

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 ISP_resume(struct platform_device *pDev)
{
	/* TG_VF_CON[0] (0x15004414[0]): VFDATA_EN. TG1 Take Picture Request. */
	MUINT32 regTG1Val = ISP_RD32((void *)(ISP_ADDR + 0x414));

	LOG_DBG("g_bPass1_On_In_Resume_TG1(%d),regTG1Val(0x%x)", g_bPass1_On_In_Resume_TG1,
		regTG1Val);

	if (g_bPass1_On_In_Resume_TG1) {
		g_bPass1_On_In_Resume_TG1 = 0;
		ISP_WR32((void *)(ISP_ADDR + 0x414), (regTG1Val | 0x01));	/* For TG1 Main sensor. */
	}

	return 0;
}

/*******************************************************************************
*
********************************************************************************/
#ifdef CONFIG_PM
MINT32 ISP_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return ISP_suspend(pdev, PMSG_SUSPEND);
}

MINT32 ISP_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return ISP_resume(pdev);
}

/* move to camera_isp_D2.h */
/*extern MVOID mt_irq_set_sens(MUINT32 irq, MUINT32 sens);*/
/*extern MVOID mt_irq_set_polarity(MUINT32 irq, MUINT32 polarity);*/
MINT32 ISP_pm_restore_noirq(struct device *device)
{
	pr_debug("calling %s()\n", __func__);
#if 0				/* fix me later */
	mt_irq_set_sens(CAMERA_ISP_IRQ0_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(CAMERA_ISP_IRQ0_ID, MT_POLARITY_LOW);
#endif
	return 0;

}

#else

#define ISP_pm_suspend NULL
#define ISP_pm_resume  NULL
#define ISP_pm_restore_noirq NULL

#endif				/*CONFIG_PM */

#ifdef CONFIG_OF
static const struct of_device_id isp_of_ids[] = {
	{.compatible = "mediatek,ISPSYS",},
	{}
};
#endif


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
********************************************************************************/
static struct platform_driver IspDriver = {

	.probe = ISP_probe,
	.remove = ISP_remove,
	.suspend = ISP_suspend,
	.resume = ISP_resume,
	.driver = {
		   .name = ISP_DEV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = isp_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &ISP_pm_ops,
#endif
		   }
};

/*******************************************************************************
*
********************************************************************************/
static ssize_t ISP_DumpRegToProc(struct file *pPage,
				char __user *pBuffer, size_t Count, loff_t *off)
{
	static MUINT32 i, j;
	static int dataState = 1, lastDataState;
	char tempStr[256];
	char tempStr2[256] = { '\0' };
	long length = 0;

	if (Count < 256) {
		LOG_ERR("BufferSize(%d) less than 256.", (int)Count);
		return 0;
	}

	switch (dataState) {
	case 0:
		dataState = lastDataState;
		break;
	case 1:
		i = 0x0;
		j = 0x64;
		length += sprintf(tempStr, "MT ISP Register\n");
		strncat(tempStr2, tempStr, length);
		length += sprintf(tempStr, "====== top ====\n");
		strncat(tempStr2, tempStr, length);
		break;
	case 2:
		i = 0x200;
		j = 0x3D8;
		length += sprintf(tempStr, "====== dma ====\n");
		strncat(tempStr2, tempStr, length);
		break;
	case 3:
		i = 0x400;
		j = 0x4EC;
		length += sprintf(tempStr, "====== tg ====\n");
		strncat(tempStr2, tempStr, length);
		break;
	case 4:
		i = 0xB00;
		j = 0xDE0;
		length += sprintf(tempStr, "====== cdp (including EIS) ====\n");
		strncat(tempStr2, tempStr, length);
		break;
	case 5:
		i = 0x4000;
		j = 0x40C0;
		break;
	case 6:
		i = 0x4100;
		j = 0x41BC;
		break;
	case 7:
		i = 0x4300;
		j = 0x4310;
		break;
	case 8:
		i = 0x43A0;
		j = 0x43B0;
		break;
	case 9:
		i = 0x4400;
		j = 0x4424;
		break;
	case 10:
		i = 0x4500;
		j = 0x4520;
		break;
	case 11:
		i = 0x4F00;
		j = 0x4F38;
		length += sprintf(tempStr, "====== 3DNR ====\n");
		strncat(tempStr2, tempStr, length);
		break;
	default:
		dataState = 0;
		lastDataState = 0;
		return 0;
	}

	LOG_INF(" i(%d), j(%d), Count(%ld), dataState(%d)", i, j,
		(long int)Count, dataState);

	for (; i <= j; i += 4) {
		if (length < 226) {/*Preserve some memory for safety*/
			length += sprintf(tempStr, "+0x%08x 0x%08x\n", (unsigned int)(ISP_ADDR + i),
			     ISP_RD32(ISP_ADDR + i));
			strncat(tempStr2, tempStr, length);
		} else {
			lastDataState = dataState;
			dataState = 0;/*a state to keep i, j value*/
			i -= 4;
			if (copy_to_user(pBuffer, tempStr2, length))
				return -EFAULT;

			return length;
		}
	}

	if (dataState <= 11) {
		if (copy_to_user(pBuffer, tempStr2, length))
			return -EFAULT;

		dataState++;
		return length;
	} else
		return 0;/*end of reading*/

}

/*******************************************************************************
*
********************************************************************************/
static ssize_t ISP_RegDebug(struct file *pFile,
			   const char __user *pBuffer, size_t  Count, loff_t *p_off)
{
	char RegBuf[64];
	MUINT32 CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
	MUINT32 Addr = 0;
	MUINT32 Data = 0;

	if ((Count <= 0) || (Count > (sizeof(RegBuf) - 1))) {
		LOG_ERR("Count(%ld) size error!", (long int)Count);
		return -EFAULT;
	}

	LOG_DBG("pFile(%p),pBuffer(%p),Count(%ld)", pFile, pBuffer, (long int)Count);

	if (copy_from_user(RegBuf, pBuffer, CopyBufSize)) {
		LOG_ERR("copy_from_user() fail.");
		return -EFAULT;
	}

	if (sscanf(RegBuf, "%x %x", &Addr, &Data) == 2) {
		ISP_WR32((ISP_ADDR_CAMINF + Addr), Data);
		LOG_ERR("Write => Addr: 0x%08X, Write Data: 0x%08X. Read Data: 0x%08X.",
			(unsigned int)(ISP_ADDR_CAMINF + Addr), Data,
			ioread32((void *)(ISP_ADDR_CAMINF + Addr)));
	} else if (kstrtou32(RegBuf, 0, &Addr) == 0) {
		LOG_ERR("Read => Addr: 0x%08X, Read Data: 0x%08X.",
			(unsigned int)(ISP_ADDR_CAMINF + Addr), ioread32((void *)(ISP_ADDR_CAMINF + Addr)));
	}

	LOG_DBG("Count(%d)", (MINT32)Count);
	return (ssize_t)Count;
}

static MUINT32 proc_regOfst;
static ssize_t CAMIO_DumpRegToProc(struct file *pPage,
				char __user *pBuffer, size_t Count, loff_t *off)
{
	char tempStr[256];
	char tempStr2[256] = {'\0'};
	long length = 0;
	static int finished;
	if (finished) {
		finished = 0;
		return 0;
	} else
		finished = 1;

	if (Count < 200) {
		LOG_ERR("BufferSize(%d) less than 256.", (int)Count);
		return 0;
	}

	length += sprintf(tempStr, "reg_0x%08X = 0x%X\n",
		(unsigned int)(ISP_ADDR_CAMINF + proc_regOfst),
		     ioread32((void *)(ISP_ADDR_CAMINF + proc_regOfst)));

	strncat(tempStr2, tempStr, length);

	if (copy_to_user(pBuffer, tempStr2, length))
		return -EFAULT;

	return length;/*end of reading*/

}

/*******************************************************************************
*
********************************************************************************/
static ssize_t CAMIO_RegDebug(struct file *pFile,
			     const char __user*pBuffer, size_t Count, loff_t *p_off)
{
	char RegBuf[64];
	MUINT32 CopyBufSize = (Count < (sizeof(RegBuf) - 1)) ? (Count) : (sizeof(RegBuf) - 1);
#if 0
	MUINT32 Addr = 0;
	MUINT32 Data = 0;
#endif
	LOG_DBG("pFile(%p),pBuffer(%p),Count(%ld)", pFile, pBuffer, (long int)Count);

	if (copy_from_user(RegBuf, pBuffer, CopyBufSize)) {
		LOG_ERR("copy_from_user() fail.");
		return -EFAULT;
	}
#if 0
	if (sscanf(RegBuf, "%x %x", &Addr, &Data) == 2) {
		proc_regOfst = Addr;
		/* ISP_WR32((void *)(GPIO_BASE + Addr), Data); //TODO: Must fixed by Device tree */
		LOG_ERR("Write => Addr: 0x%08X, Write Data: 0x%08X. Read Data: 0x%08X.",
			GPIO_BASE + Addr, Data, ioread32((GPIO_BASE + Addr)));
	} else if (sscanf(RegBuf, "%x", &Addr) == 1) {
		proc_regOfst = Addr;
		LOG_ERR("Read => Addr: 0x%08X, Read Data: 0x%08X.", GPIO_BASE + Addr,
			ioread32((GPIO_BASE + Addr)));
	}
#endif

	LOG_DBG("Count(%ld)", (long int) Count);
	return ((ssize_t)Count);
}

/*******************************************************************************
*
********************************************************************************/
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
********************************************************************************/
bool ISP_RegCallback(ISP_CALLBACK_STRUCT *pCallback)
{
	if (pCallback == NULL) {
		LOG_ERR("pCallback is null");
		return false;
	}

	if (pCallback->Func == NULL) {
		LOG_ERR("Func is null");
		return false;
	}

	LOG_DBG("Type(%d)", pCallback->Type);
	if (pCallback->Type >= 0 && pCallback->Type < ISP_CALLBACK_AMOUNT)
		g_IspInfo.Callback[pCallback->Type].Func = pCallback->Func;

	return true;
}
EXPORT_SYMBOL(ISP_RegCallback);

/*******************************************************************************
*
********************************************************************************/
bool ISP_UnregCallback(ISP_CALLBACK_ENUM Type)
{
	if (Type >= ISP_CALLBACK_AMOUNT) {
		LOG_ERR("Type(%d) must smaller than %d", Type, ISP_CALLBACK_AMOUNT);
		return false;
	}

	LOG_DBG("Type(%d)", Type);
	g_IspInfo.Callback[Type].Func = NULL;

	return true;
}
EXPORT_SYMBOL(ISP_UnregCallback);

/*******************************************************************************
*
********************************************************************************/
void ISP_MCLK1_EN(bool En)
{
	MUINT32 temp = 0;

		if (1 == En)
			mMclk1User++;
		else {
			mMclk1User--;
			if (mMclk1User <= 0)
				mMclk1User = 0;
		}

		temp = ISP_RD32(ISP_ADDR + 0x4200);
		if (En) {
			if (mMclk1User > 0) {
				temp |= 0x20000000;
				ISP_WR32(ISP_ADDR + 0x4200, temp);
			}
		} else {
			if (mMclk1User == 0) {
				temp &= 0xDFFFFFFF;
				ISP_WR32(ISP_ADDR + 0x4200, temp);
			}
		}
		temp = ISP_RD32(ISP_ADDR + 0x4200);
		LOG_DBG("ISP_MCLK1_EN(%d), mMclk1User(%d)", temp, mMclk1User);

}
EXPORT_SYMBOL(ISP_MCLK1_EN);

/*******************************************************************************
*
********************************************************************************/

bool ISP_ControlMdpClock(bool en)
{
	if (en == MTRUE) {
		cmdqCoreRegisterCB(CMDQ_GROUP_ISP,
				   ISP_MDPClockOnCallback,
				   ISP_MDPDumpCallback,
				   ISP_MDPResetCallback, ISP_MDPClockOffCallback);
	} else {
		/* Unregister ISP callback */
		cmdqCoreRegisterCB(CMDQ_GROUP_ISP, NULL, NULL, NULL, NULL);
	}

	return MTRUE;
}


int32_t ISP_MDPClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("ISP_MDPClockOnCallback"); */

	ISP_EnableClock(MTRUE, 0);

	return 0;
}

int32_t ISP_MDPDumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("ISP_MDPDumpCallback");

	ISP_DumpReg();

	return 0;
}

int32_t ISP_MDPResetCallback(uint64_t engineFlag)
{
	LOG_DBG("[Warning]Reset ISP");

	ISP_Reset();

	return 0;
}

int32_t ISP_MDPClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("ISP_MDPClockOffCallback"); */

	ISP_EnableClock(MFALSE, 0);

	return 0;
}

m4u_callback_ret_t ISP_M4U_TranslationFault_callback(int port, unsigned int mva, void *data)
{
	LOG_DBG("[ISP_M4U]fault call port=%d, mva=0x%x", port, mva);

	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x0004),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x0004));
	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x0008),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x0008));
	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x000C),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x000C));
	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x0010),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x0010));
	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x0074),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x0074));
	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x0078),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x0078));
	LOG_DBG("[TF_%d]0x%08X %08X", port, (unsigned int)(ISP_TPIPE_ADDR + 0x007c),
		(unsigned int)ISP_RD32(ISP_ADDR + 0x007c));

	switch (port) {
	case M4U_PORT_IMGO:
		LOG_DBG("[TF_IMGO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0300),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0300));
		LOG_DBG("[TF_IMGO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0304),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0304));
		LOG_DBG("[TF_IMGO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0308),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0308));
		LOG_DBG("[TF_IMGO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x030C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x030C));
		LOG_DBG("[TF_IMGO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0310),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0310));
		LOG_DBG("[TF_IMGO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x031C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x031C));
		break;
	case M4U_PORT_IMGO2O:
		LOG_DBG("[TF_IMG2O]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0320),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0320));
		LOG_DBG("[TF_IMG2O]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0324),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0324));
		LOG_DBG("[TF_IMG2O]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0328),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0328));
		LOG_DBG("[TF_IMG2O]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x032C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x032C));
		LOG_DBG("[TF_IMG2O]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0330),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0330));
		LOG_DBG("[TF_IMG2O]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x033C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x033C));
		break;
	case M4U_PORT_LSCI:
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00A8),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00A8));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00B8),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00B8));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00BC),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00BC));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00E8),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00E8));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00EC),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00EC));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00F0),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00F0));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x026C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x026C));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0270),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0270));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0274),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0274));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0278),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0278));
		LOG_DBG("[TF_LSCI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x027c),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x027c));
		break;
	case M4U_PORT_CAM_IMGI:
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0050),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0050));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0054),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0054));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00AC),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00AC));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x00B0),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x00B0));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0230),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0230));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0234),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0234));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0238),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0238));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x023C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x023C));
		LOG_DBG("[TF_IMGI]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0240),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0240));
		break;
	case M4U_PORT_CAM_ESFKO:
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x035C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x035C));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0360),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0360));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0364),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0364));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0368),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0368));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x036C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x036C));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0370),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0370));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0374),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0374));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0378),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0378));
		LOG_DBG("[TF_ESFKO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x037C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x037C));
		break;
	case M4U_PORT_CAM_AAO:
		LOG_DBG("[TF_AAO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0388),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0388));
		LOG_DBG("[TF_AAO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x038C),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x038C));
		LOG_DBG("[TF_AAO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0390),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0390));
		LOG_DBG("[TF_AAO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0394),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0394));
		LOG_DBG("[TF_AAO]0x%08X %08X", (unsigned int)(ISP_TPIPE_ADDR + 0x0398),
			(unsigned int)ISP_RD32(ISP_ADDR + 0x0398));
		break;
	default:
		break;
	}

	return M4U_CALLBACK_HANDLED;
}

/*******************************************************************************
*
********************************************************************************/
static MINT32 __init ISP_Init(MVOID)
{
	MINT32 i;
	MINT32 Ret = 0;
#if 0
	struct proc_dir_entry *pEntry;
#endif
	LOG_DBG("+");

	Ret = platform_driver_register(&IspDriver);
	if ((Ret) < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}
#if 1				/* linux-3.10 procfs API changed */
	proc_create("driver/isp_reg", 0, NULL, &fcameraisp_proc_fops);
	proc_create("driver/camio_reg", 0, NULL, &fcameraio_proc_fops);
#else
	pEntry = create_proc_entry("driver/isp_reg", 0, NULL);
	if (pEntry) {
		pEntry->read_proc = ISP_DumpRegToProc;
		pEntry->write_proc = ISP_RegDebug;
	} else {
		LOG_ERR("add /proc/driver/isp_reg entry fail");
	}

	pEntry = create_proc_entry("driver/camio_reg", 0, NULL);
	if (pEntry) {
		pEntry->read_proc = CAMIO_DumpRegToProc;
		pEntry->write_proc = CAMIO_RegDebug;
	} else {
		LOG_ERR("add /proc/driver/camio_reg entry fail");
	}
#endif
	/* allocate a memory area with kmalloc. Will be rounded up to a page boundary */
	/* RT_BUF_TBL_NPAGES*4096(1page) = 64k Bytes */
	g_pBuf_kmalloc = kmalloc((RT_BUF_TBL_NPAGES + 2) * PAGE_SIZE, GFP_KERNEL);
	if ((g_pBuf_kmalloc) == NULL) {
		return -ENOMEM;

	}

	memset(g_pBuf_kmalloc, 0x00, RT_BUF_TBL_NPAGES * PAGE_SIZE);
	/*  */
	LOG_INF("register isp callback for MDP,is_v3(%d)", CAM_HAL_VER_IS3);
	ISP_ControlMdpClock(MTRUE);

	/* round it up to the page bondary */
	g_pTbl_RTBuf = (MINT32 *) ((((unsigned long)g_pBuf_kmalloc) + PAGE_SIZE - 1) & PAGE_MASK);
	g_pstRTBuf = (ISP_RT_BUF_STRUCT *) g_pTbl_RTBuf;
	g_pstRTBuf->state = ISP_RTBC_STATE_INIT;

	/* mark the pages reserved */
	for (i = 0; i < RT_BUF_TBL_NPAGES * PAGE_SIZE; i += PAGE_SIZE) {
		SetPageReserved(virt_to_page(((unsigned long)g_pTbl_RTBuf) + i));
	}

	/* Register M4U callback dump */
	LOG_DBG("register M4U callback dump");
	m4u_register_fault_callback(M4U_PORT_IMGO, ISP_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_IMGO2O, ISP_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_LSCI, ISP_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_CAM_IMGI, ISP_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_CAM_ESFKO, ISP_M4U_TranslationFault_callback, NULL);
	m4u_register_fault_callback(M4U_PORT_CAM_AAO, ISP_M4U_TranslationFault_callback, NULL);

	ISP_Init_FrmB();

	/*LOG_DBG("Ret(%d)", Ret);*/
	return Ret;
}

/*******************************************************************************
*
********************************************************************************/
static MVOID __exit ISP_Exit(MVOID)
{
	MINT32 i;

	/*LOG_DBG("+");*/

	platform_driver_unregister(&IspDriver);
	/*  */
	LOG_INF("+ unregister isp callback for MDP,is_v3(%d)", CAM_HAL_VER_IS3);
	ISP_ControlMdpClock(false);

	/* unreserve the pages */
	for (i = 0; i < RT_BUF_TBL_NPAGES * PAGE_SIZE; i += PAGE_SIZE) {
		SetPageReserved(virt_to_page(((unsigned long)g_pTbl_RTBuf) + i));
	}

	/* free the memory areas */
	kfree(g_pBuf_kmalloc);

	ISP_Exit_FRMB();

	/*LOG_DBG("-");*/
}

/*******************************************************************************
*
********************************************************************************/
module_init(ISP_Init);
module_exit(ISP_Exit);
MODULE_DESCRIPTION("Camera ISP driver");
MODULE_AUTHOR("ME3");
MODULE_LICENSE("GPL");

