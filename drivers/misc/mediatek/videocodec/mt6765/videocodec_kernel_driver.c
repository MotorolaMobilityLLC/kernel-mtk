/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
/* #include <mach/irqs.h> */
/* #include <mach/x_define_irq.h> */
#include <linux/wait.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/suspend.h>
#include <linux/pm_wakeup.h>
#include <mt-plat/dma.h>
#include "mt-plat/sync_write.h"

#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#else
#include "mach/mt_clkmgr.h"
#endif

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#include <mach/diso.h>
#endif

#include "videocodec_kernel_driver.h"
#include "../videocodec_kernel.h"
#include "smi_public.h"
#include <asm/cacheflush.h>
#include <linux/io.h>
#include <asm/sizes.h>
#include "val_types_private.h"
#include "hal_types_private.h"
#include "val_api_private.h"
#include "val_log.h"
#include "drv_api.h"

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#if IS_ENABLED(CONFIG_COMPAT)
#include <linux/uaccess.h>
#include <linux/compat.h>
#endif

#define VDO_HW_WRITE(ptr, data)     mt_reg_sync_writel(data, ptr)
#define VDO_HW_READ(ptr)            readl((void __iomem *)ptr)

#define VCODEC_DEVNAME     "Vcodec"
#define VCODEC_DEV_MAJOR_NUMBER 160   /* 189 */

static dev_t vcodec_devno = MKDEV(VCODEC_DEV_MAJOR_NUMBER, 0);
static struct cdev *vcodec_cdev;
static struct class *vcodec_class;
static struct device *vcodec_device;

#ifndef VCODEC_FPGAPORTING
#ifndef CONFIG_MTK_CLKMGR
static struct clk *clk_MT_CG_VDEC;              /* VENC_GCON_VDEC */
static struct clk *clk_MT_CG_VENC;              /* VENC_GCON_VENC */
static struct clk *clk_MT_SCP_SYS_VCODEC;       /* SCP_SYS_VCODEC */
static struct clk *clk_MT_SCP_SYS_DISP;		/* SCP_SYS_DISP */

#endif
#endif

#ifndef CONFIG_MTK_SMI_EXT
static struct clk *clk_MT_CG_GALS_VENC2MM;
static struct clk *clk_MT_CG_GALS_VDEC2MM;
static struct clk *clk_MT_CG_SMI_COMMON;
#endif

static DEFINE_MUTEX(IsOpenedLock);
static DEFINE_MUTEX(PWRLock);
static DEFINE_MUTEX(HWLock);
static DEFINE_MUTEX(EncEMILock);
static DEFINE_MUTEX(DecEMILock);
static DEFINE_MUTEX(DriverOpenCountLock);
static DEFINE_MUTEX(HWLockEventTimeoutLock);

static DEFINE_MUTEX(VdecPWRLock);
static DEFINE_MUTEX(VencPWRLock);
static DEFINE_MUTEX(LogCountLock);

static DEFINE_SPINLOCK(DecIsrLock);
static DEFINE_SPINLOCK(EncIsrLock);
static DEFINE_SPINLOCK(LockDecHWCountLock);
static DEFINE_SPINLOCK(LockEncHWCountLock);
static DEFINE_SPINLOCK(DecISRCountLock);
static DEFINE_SPINLOCK(EncISRCountLock);


static struct VAL_EVENT_T HWLockEvent;   /* mutex : HWLockEventTimeoutLock */
static struct VAL_EVENT_T DecIsrEvent;    /* mutex : HWLockEventTimeoutLock */
static struct VAL_EVENT_T EncIsrEvent;    /* mutex : HWLockEventTimeoutLock */
static int Driver_Open_Count;         /* mutex : DriverOpenCountLock */
static unsigned int gu4PWRCounter;      /* mutex : PWRLock */
static unsigned int gu4EncEMICounter;   /* mutex : EncEMILock */
static unsigned int gu4DecEMICounter;   /* mutex : DecEMILock */
static char bIsOpened = VAL_FALSE;    /* mutex : IsOpenedLock */
static unsigned int gu4HwVencIrqStatus; /* hardware VENC IRQ status(VP8/H264) */

static unsigned int gu4VdecPWRCounter;  /* mutex : VdecPWRLock */
static unsigned int gu4VencPWRCounter;  /* mutex : VencPWRLock */

static unsigned int gu4LogCountUser;  /* mutex : LogCountLock */
static unsigned int gu4LogCount;

static unsigned int gLockTimeOutCount;

static unsigned int gu4VdecLockThreadId;

#define USE_WAKELOCK 0

#if USE_WAKELOCK == 1
static struct wakeup_source v_wakeup_src;
#elif USE_WAKELOCK == 0
static unsigned int is_entering_suspend;
#endif

/* #define VCODEC_DEBUG */
#ifdef VCODEC_DEBUG
#undef VCODEC_DEBUG
#define VCODEC_DEBUG pr_info
#undef pr_debug
#define pr_debug  pr_info
#else
#define VCODEC_DEBUG(...)
#undef pr_debug
#define pr_debug(...)
#endif

/* VENC physical base address */
#undef VENC_BASE
#define VENC_BASE       0x17002000
#define VENC_REGION     0x1000

/* VDEC virtual base address */
#define VDEC_BASE_PHY   0x17000000
#define VDEC_REGION     0x50000

#define HW_BASE         0x7FFF000
#define HW_REGION       0x2000

#define INFO_BASE       0x10000000
#define INFO_REGION     0x1000

#if 0
#define VENC_IRQ_STATUS_addr        (VENC_BASE + 0x05C)
#define VENC_IRQ_ACK_addr           (VENC_BASE + 0x060)
#define VENC_MP4_IRQ_ACK_addr       (VENC_BASE + 0x678)
#define VENC_MP4_IRQ_STATUS_addr    (VENC_BASE + 0x67C)
#define VENC_ZERO_COEF_COUNT_addr   (VENC_BASE + 0x688)
#define VENC_BYTE_COUNT_addr        (VENC_BASE + 0x680)
#define VENC_MP4_IRQ_ENABLE_addr    (VENC_BASE + 0x668)

#define VENC_MP4_STATUS_addr        (VENC_BASE + 0x664)
#define VENC_MP4_MVQP_STATUS_addr   (VENC_BASE + 0x6E4)
#endif


#define VENC_IRQ_STATUS_SPS         0x1
#define VENC_IRQ_STATUS_PPS         0x2
#define VENC_IRQ_STATUS_FRM         0x4
#define VENC_IRQ_STATUS_DRAM        0x8
#define VENC_IRQ_STATUS_PAUSE       0x10
#define VENC_IRQ_STATUS_SWITCH      0x20

#if 0
/* VDEC virtual base address */
#define VDEC_MISC_BASE  (VDEC_BASE + 0x0000)
#define VDEC_VLD_BASE   (VDEC_BASE + 0x1000)
#endif

#define DRAM_DONE_POLLING_LIMIT 20000

void *KVA_VENC_IRQ_ACK_ADDR, *KVA_VENC_IRQ_STATUS_ADDR, *KVA_VENC_BASE;
void *KVA_VDEC_MISC_BASE, *KVA_VDEC_VLD_BASE;
void *KVA_VDEC_BASE, *KVA_VDEC_GCON_BASE;
unsigned int VENC_IRQ_ID, VDEC_IRQ_ID;

extern void __attribute__((weak)) met_mmsys_tag(const char *tag,
					unsigned int value);

/* #define KS_POWER_WORKAROUND */

/* extern unsigned long pmem_user_v2p_video(unsigned long va); */

#include <mtk_smi.h>
/* disable temporary for alaska DVT verification, smi seems not ready */
#define ENABLE_MMDVFS_VDEC
#ifdef ENABLE_MMDVFS_VDEC
/* <--- MM DVFS related */
#include <mmdvfs_config_util.h>
#endif

unsigned int TimeDiffMs(struct VAL_TIME_T timeOld, struct VAL_TIME_T timeNew)
{
	/* pr_info ("@@ timeOld(%d, %d), timeNew(%d, %d)", */
	/* timeOld.u4Sec, timeOld.u4uSec, timeNew.u4Sec, timeNew.u4uSec); */
	return ((((timeNew.u4Sec - timeOld.u4Sec) * 1000000) + timeNew.u4uSec) -
		 timeOld.u4uSec) / 1000;
}

void *mt_venc_base_get(void)
{
	return (void *)KVA_VENC_BASE;
}
EXPORT_SYMBOL(mt_venc_base_get);

void *mt_vdec_base_get(void)
{
	return (void *)KVA_VDEC_BASE;
}
EXPORT_SYMBOL(mt_vdec_base_get);
/* void vdec_log_status(void)
 *{
 *	unsigned int u4DataStatusMain = 0;
 *	unsigned int i = 0;
 *
 *	for (i = 45; i < 72; i++) {
 *		if (i == 45 || i == 46 || i == 52 || i == 58 ||
 *			i == 59 || i == 61 || i == 62 || i == 63 ||
 *			i == 71) {
 *			u4DataStatusMain = VDO_HW_READ(KVA_VDEC_VLD_BASE+(i*4));
 *			pr_info("[VCODEC][DUMP] "
 *				"VLD_%d = %x\n", i, u4DataStatusMain);
 *		}
 *	}
 *
 *	for (i = 66; i < 80; i++) {
 *		u4DataStatusMain = VDO_HW_READ(KVA_VDEC_MISC_BASE+(i*4));
 *		pr_info("[VCODEC][DUMP] "
 *				"MISC_%d = %x\n", i, u4DataStatusMain);
 *	}
 *
 *}
 */

/* TODO: Check register for shared vdec/venc */
void vdec_polling_status(void)
{
	unsigned int u4DataStatusMain = 0;
	unsigned int u4DataStatus = 0;
	unsigned int u4CgStatus = 0;
	unsigned int u4Counter = 0;
	void *u4Reg = 0;

	u4CgStatus = VDO_HW_READ(KVA_VDEC_GCON_BASE);
	u4DataStatusMain = VDO_HW_READ(KVA_VDEC_VLD_BASE+(61*4));

	while ((u4CgStatus != 0) && (u4DataStatusMain & (1<<15)) &&
		((u4DataStatusMain & 1) != 1)) {
		u4CgStatus = VDO_HW_READ(KVA_VDEC_GCON_BASE);
		u4DataStatusMain = VDO_HW_READ(KVA_VDEC_VLD_BASE+(61*4));
		if (u4Counter++ > DRAM_DONE_POLLING_LIMIT) {
			unsigned int u4IntStatus = 0;
			unsigned int i = 0;

			pr_err("[ERROR] Leftover data before power down");
			for (i = 45; i < 72; i++) {
				if (i == 45 || i == 46 || i == 52 || i == 58 ||
					i == 59 || i == 61 || i == 62 ||
					i == 63 || i == 71){
					u4Reg = KVA_VDEC_VLD_BASE+(i*4);
					u4IntStatus = VDO_HW_READ(u4Reg);
					pr_err("[VCODEC] VLD_%d = %x\n",
						i, u4IntStatus);
				}
			}

			for (i = 66; i < 80; i++) {
				u4Reg = KVA_VDEC_MISC_BASE+(i*4);
				u4DataStatus = VDO_HW_READ(u4Reg);
				pr_info("[VCODEC] MISC_%d = %x\n",
						i, u4DataStatus);
			}
#ifdef CONFIG_MTK_SMI_EXT
			smi_debug_bus_hang_detect(SMI_PARAM_BUS_OPTIMIZATION,
							1, 0, 1);
#endif
			mmsys_cg_check();

			u4Counter = 0;
			WARN_ON(1);
		}
	}
	/* pr_info("u4Counter %d\n", u4Counter); */

}

void vdec_power_on(void)
{
	int ret = 0;

	mutex_lock(&VdecPWRLock);
	gu4VdecPWRCounter++;
	mutex_unlock(&VdecPWRLock);
	ret = 0;

	ret = clk_prepare_enable(clk_MT_SCP_SYS_DISP);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VDEC] MT_SCP_SYS_DISP is not on, ret = %d\n",
				ret);
	}

#if defined(CONFIG_MTK_SMI_EXT)
	smi_bus_prepare_enable(SMI_LARB1_REG_INDX, "VDEC", true);
#else
	ret = clk_prepare_enable(clk_MT_CG_SMI_COMMON);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VCODEC] MT_CG_SMI_COMMON is not on, ret = %d\n",
				ret);
		}

	ret = clk_prepare_enable(clk_MT_CG_GALS_VDEC2MM);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VCODEC] MT_CG_GALS_VDEC2MM is not on, ret = %d\n",
				ret);
		}
#endif

	ret = clk_prepare_enable(clk_MT_SCP_SYS_VCODEC);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VCODEC] MT_SCP_SYS_VDE is not on, ret = %d\n", ret);
	}

	ret = clk_prepare_enable(clk_MT_CG_VDEC);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VCODEC] MT_CG_VDEC is not on, ret = %d\n", ret);
	}

}

void vdec_power_off(void)
{
	/* bianco VCODEC_SEL reset */
	do {
		VDO_HW_WRITE(KVA_VDEC_GCON_BASE + 0x20, 0);
	} while (VDO_HW_READ(KVA_VDEC_GCON_BASE + 0x20) != 0);

	mutex_lock(&VdecPWRLock);
	if (gu4VdecPWRCounter == 0) {
		pr_debug("[VCODEC] gu4VdecPWRCounter = 0\n");
	} else {

		vdec_polling_status();
		gu4VdecPWRCounter--;
		clk_disable_unprepare(clk_MT_CG_VDEC);
		clk_disable_unprepare(clk_MT_SCP_SYS_VCODEC);
#if defined(CONFIG_MTK_SMI_EXT)
		smi_bus_disable_unprepare(SMI_LARB1_REG_INDX, "VDEC", true);
#else
		clk_disable_unprepare(clk_MT_CG_GALS_VDEC2MM);
		clk_disable_unprepare(clk_MT_CG_SMI_COMMON);
#endif
		clk_disable_unprepare(clk_MT_SCP_SYS_DISP);
	}
	mutex_unlock(&VdecPWRLock);
}

void venc_power_on(void)
{
	int ret = 0;

	mutex_lock(&VencPWRLock);
	gu4VencPWRCounter++;
	mutex_unlock(&VencPWRLock);
	ret = 0;

	pr_debug("[VCODEC] venc_power_on +\n");

	ret = clk_prepare_enable(clk_MT_SCP_SYS_DISP);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VENC] MT_SCP_SYS_DISP is not on, ret = %d\n",
				ret);
	}

#if defined(CONFIG_MTK_SMI_EXT)
	smi_bus_prepare_enable(SMI_LARB1_REG_INDX, "VENC", true);
#else
	ret = clk_prepare_enable(clk_MT_CG_SMI_COMMON);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VENC] MT_CG_SMI_COMMON is not on, ret = %d\n",
				ret);
	}

	ret = clk_prepare_enable(clk_MT_CG_GALS_VENC2MM);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VENC] MT_CG_GALS_VENC2MM is not on, ret = %d\n",
			ret);
	}
#endif

	ret = clk_prepare_enable(clk_MT_SCP_SYS_VCODEC);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VENC] MT_SCP_SYS_VEN is not on, ret = %d\n",
				ret);
	}

	ret = clk_prepare_enable(clk_MT_CG_VENC);
	if (ret) {
		/* print error log & error handling */
		pr_info("[VENC] MT_CG_VENC is not on, ret = %d\n",
				ret);
	}

	pr_debug("[VENC] venc_power_on -\n");
}

void venc_power_off(void)
{
	mutex_lock(&VencPWRLock);
	if (gu4VencPWRCounter == 0) {
		pr_debug("[VENC] gu4VencPWRCounter = 0\n");
	} else {
		gu4VencPWRCounter--;
		pr_debug("[VENC] venc_power_off +\n");
		clk_disable_unprepare(clk_MT_CG_VENC);
		clk_disable_unprepare(clk_MT_SCP_SYS_VCODEC);
#if defined(CONFIG_MTK_SMI_EXT)
		smi_bus_disable_unprepare(SMI_LARB7_REG_INDX, "VENC", true);
#else
		clk_disable_unprepare(clk_MT_CG_GALS_VENC2MM);
		clk_disable_unprepare(clk_MT_CG_SMI_COMMON);
#endif
		clk_disable_unprepare(clk_MT_SCP_SYS_DISP);
		pr_debug("[VENC] venc_power_off -\n");
	}
	mutex_unlock(&VencPWRLock);
}

void vdec_break(void)
{
	unsigned int i;

	/* Step 1: set vdec_break */
	VDO_HW_WRITE(KVA_VDEC_MISC_BASE + 64*4, 0x1);

	/* Step 2: monitor status vdec_break_ok */
	for (i = 0; i < 5000; i++) {
		if ((VDO_HW_READ(KVA_VDEC_MISC_BASE + 65*4) & 0x11) == 0x11) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			break;
		}
	}

	if (i >= 5000) {
		unsigned int j;
		unsigned int u4DataStatus = 0;

		pr_info("Leftover data access before powering down\n");

		for (j = 68; j < 80; j++) {
			u4DataStatus = VDO_HW_READ(KVA_VDEC_MISC_BASE+(j*4));
			pr_info("[VCODEC][DUMP] MISC_%d = 0x%08x",
				j, u4DataStatus);
		}
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(45*4));
		pr_info("[VCODEC][DUMP] VLD_45 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(46*4));
		pr_info("[VCODEC][DUMP] VLD_46 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(52*4));
		pr_info("[VCODEC][DUMP] VLD_52 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(58*4));
		pr_info("[VCODEC][DUMP] VLD_58 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(59*4));
		pr_info("[VCODEC][DUMP] VLD_59 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(61*4));
		pr_info("[VCODEC][DUMP] VLD_61 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(62*4));
		pr_info("[VCODEC][DUMP] VLD_62 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(63*4));
		pr_info("[VCODEC][DUMP] VLD_63 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_BASE+(71*4));
		pr_info("[VCODEC][DUMP] VLD_71 = 0x%08x", u4DataStatus);
		u4DataStatus = VDO_HW_READ(KVA_VDEC_MISC_BASE+(66*4));
		pr_info("[VCODEC][DUMP] MISC_66 = 0x%08x", u4DataStatus);
	}

	/* Step 3: software reset */
	VDO_HW_WRITE(KVA_VDEC_BASE + 66*4, 0x1);
	VDO_HW_WRITE(KVA_VDEC_BASE + 66*4, 0x0);
}

void venc_break(void)
{
	unsigned int i;
	void *VENC_SW_PAUSE   = KVA_VENC_BASE + 0xAC;
	void *VENC_IRQ_STATUS = KVA_VENC_BASE + 0x5C;
	void *VENC_SW_HRST_N  = KVA_VENC_BASE + 0xA8;
	void *VENC_IRQ_ACK    = KVA_VENC_BASE + 0x60;

	/* Step 1: raise pause hardware signal */
	VDO_HW_WRITE(VENC_SW_PAUSE, 0x1);

	/* Step 2: assume software can only tolerate 5000 APB read time. */
	for (i = 0; i < 5000; i++) {
		if (VDO_HW_READ(VENC_IRQ_STATUS) & 0x10) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			break;
		}
	}

	/* Step 3: Lower pause hardware signal and lower software hard reset
	 * signal
	 */
	if (i >= 5000) {
		VDO_HW_WRITE(VENC_SW_PAUSE, 0x0);
		VDO_HW_WRITE(VENC_SW_HRST_N, 0x0);
		VDO_HW_READ(VENC_SW_HRST_N);
	}

	/* Step 4: Lower software hard reset signal and lower pause hardware
	 * signal
	 */
	else {
		VDO_HW_WRITE(VENC_SW_HRST_N, 0x0);
		VDO_HW_READ(VENC_SW_HRST_N);
		VDO_HW_WRITE(VENC_SW_PAUSE, 0x0);
	}

	/* Step 5: Raise software hard reset signal */
	VDO_HW_WRITE(VENC_SW_HRST_N, 0x1);
	VDO_HW_READ(VENC_SW_HRST_N);
	/* Step 6: Clear pause status */
	VDO_HW_WRITE(VENC_IRQ_ACK, 0x10);
}

void dec_isr(void)
{
	enum VAL_RESULT_T eValRet;
	unsigned long ulFlags, ulFlagsISR, ulFlagsLockHW;

	unsigned int u4TempDecISRCount = 0;
	unsigned int u4TempLockDecHWCount = 0;
	unsigned int u4DecDoneStatus = 0;

	u4DecDoneStatus = VDO_HW_READ(KVA_VDEC_MISC_BASE+0xA4);
	if ((u4DecDoneStatus & (0x1 << 16)) != 0x10000) {
		pr_info("[VDEC] DEC ISR, Dec done is not 0x1 (0x%08x)",
				u4DecDoneStatus);
		return;
	}


	spin_lock_irqsave(&DecISRCountLock, ulFlagsISR);
	gu4DecISRCount++;
	u4TempDecISRCount = gu4DecISRCount;
	spin_unlock_irqrestore(&DecISRCountLock, ulFlagsISR);

	spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
	u4TempLockDecHWCount = gu4LockDecHWCount;
	spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);

	if (u4TempDecISRCount != u4TempLockDecHWCount) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 * pr_info("[INFO] Dec ISRCount: 0x%x, "
		 * "LockHWCount:0x%x\n",
		 * u4TempDecISRCount, u4TempLockDecHWCount);
		 */
	}

	/* Clear interrupt */
	VDO_HW_WRITE(KVA_VDEC_MISC_BASE+41*4,
		VDO_HW_READ(KVA_VDEC_MISC_BASE + 41*4) | 0x11);
	VDO_HW_WRITE(KVA_VDEC_MISC_BASE+41*4,
		VDO_HW_READ(KVA_VDEC_MISC_BASE + 41*4) & ~0x10);


	spin_lock_irqsave(&DecIsrLock, ulFlags);
	eValRet = eVideoSetEvent(&DecIsrEvent, sizeof(struct VAL_EVENT_T));
	if (eValRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VDEC] ISR set DecIsrEvent error\n");
	}
	spin_unlock_irqrestore(&DecIsrLock, ulFlags);
}


void enc_isr(void)
{
	enum VAL_RESULT_T eValRet;
	unsigned long ulFlagsISR, ulFlagsLockHW;


	unsigned int u4TempEncISRCount = 0;
	unsigned int u4TempLockEncHWCount = 0;
	/* ---------------------- */

	spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
	gu4EncISRCount++;
	u4TempEncISRCount = gu4EncISRCount;
	spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);

	spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
	u4TempLockEncHWCount = gu4LockEncHWCount;
	spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);

	if (u4TempEncISRCount != u4TempLockEncHWCount) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 * pr_info("[INFO] Enc ISRCount: 0x%xa, "
		 *	"LockHWCount:0x%x\n",
		 * u4TempEncISRCount, u4TempLockEncHWCount);
		 */
	}

	if (CodecHWLock.pvHandle == 0) {
		pr_info("[VENC] NO one Lock Enc HW, please check!!\n");

		/* Clear all status */
		/* VDO_HW_WRITE(KVA_VENC_MP4_IRQ_ACK_ADDR, 1); */
		VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_PAUSE);
		/* VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
		 * VENC_IRQ_STATUS_DRAM_VP8);
		 */
		VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_SWITCH);
		VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_DRAM);
		VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_SPS);
		VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_PPS);
		VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR, VENC_IRQ_STATUS_FRM);
		return;
	}

	if (CodecHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC) {/* hardwire */
		gu4HwVencIrqStatus = VDO_HW_READ(KVA_VENC_IRQ_STATUS_ADDR);
		if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_PAUSE) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
					VENC_IRQ_STATUS_PAUSE);
		}
		if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_SWITCH) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
					VENC_IRQ_STATUS_SWITCH);
		}
		if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_DRAM) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
					VENC_IRQ_STATUS_DRAM);
		}
		if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_SPS) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
					VENC_IRQ_STATUS_SPS);
		}
		if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_PPS) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
					VENC_IRQ_STATUS_PPS);
		}
		if (gu4HwVencIrqStatus & VENC_IRQ_STATUS_FRM) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			VDO_HW_WRITE(KVA_VENC_IRQ_ACK_ADDR,
					VENC_IRQ_STATUS_FRM);
		}
	} else {
		pr_info("[VENC] Invalid lock holder driver type = %d\n",
				CodecHWLock.eDriverType);
	}

	eValRet = eVideoSetEvent(&EncIsrEvent, sizeof(struct VAL_EVENT_T));
	if (eValRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VENC] ISR set EncIsrEvent error\n");
	}
}

static irqreturn_t video_intr_dlr(int irq, void *dev_id)
{
	if (met_mmsys_tag)
		met_mmsys_tag("VDEC", CodecHWLock.eDriverType | 0x100);

	dec_isr();
	return IRQ_HANDLED;
}

static irqreturn_t video_intr_dlr2(int irq, void *dev_id)
{
	enc_isr();
	return IRQ_HANDLED;
}

static long vcodec_lockhw_dec_fail(struct VAL_HW_LOCK_T rHWLock,
				unsigned int FirstUseDecHW)
{
	pr_info("VCODEC_LOCKHW, HWLockEvent TimeOut, CurrentTID = %d\n",
			current->pid);
	if (FirstUseDecHW != 1) {
		mutex_lock(&HWLock);
		if (CodecHWLock.pvHandle == 0) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			pr_info("VCODEC_LOCKHW, mediaserver may restarted");
		} else {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			pr_info("VCODEC_LOCKHW, someone use HW");
		}
		mutex_unlock(&HWLock);
	}

	return 0;
}

#define ENC_LOCK_FLOG "venc locked HW timeout 0x\%lx,\%lx,0x\%lx, type:\%d"
#define ENC_LOCK_FLOG2 "someone locked HW already 0x\%lx,\%lx,0x\%lx, type:\%d"

static long vcodec_lockhw_enc_fail(struct VAL_HW_LOCK_T rHWLock,
				unsigned int FirstUseEncHW)
{
	pr_info("VENC_LOCKHW HWLockEvent TimeOut, CurrentTID = %d\n",
			current->pid);

	if (FirstUseEncHW != 1) {
		mutex_lock(&HWLock);
		if (CodecHWLock.pvHandle == 0) {
			pr_info("VENC_LOCKHW, mediaserver may restarted");
		} else {
			pr_info("VENC_LOCKHW, someone use HW (%d)\n",
				 gLockTimeOutCount);
			++gLockTimeOutCount;

			if (gLockTimeOutCount > 30) {
				pr_info("VENC_LOCKHW - ID %d fail\n",
						current->pid);
				pr_info(ENC_LOCK_FLOG,
				(unsigned long)CodecHWLock.pvHandle,
				pmem_user_v2p_video(
					(unsigned long)rHWLock.pvHandle),
				(unsigned long)rHWLock.pvHandle,
				rHWLock.eDriverType);
				gLockTimeOutCount = 0;
				mutex_unlock(&HWLock);
				return -EFAULT;
			}

			if (rHWLock.u4TimeoutMs == 0) {
				pr_info("VENC_LOCKHW - ID %d fail\n",
						current->pid);
				pr_info(ENC_LOCK_FLOG2,
				(unsigned long)CodecHWLock.pvHandle,
				 pmem_user_v2p_video(
					(unsigned long)rHWLock.pvHandle),
				(unsigned long)rHWLock.pvHandle,
				 rHWLock.eDriverType);
				gLockTimeOutCount = 0;
				mutex_unlock(&HWLock);
				return -EFAULT;
			}
		}
		mutex_unlock(&HWLock);
	}

	return 0;
}

static long vcodec_lockhw_vdec(struct VAL_HW_LOCK_T *pHWLock, char *bLockedHW)
{
	unsigned int FirstUseDecHW = 0;
	unsigned long ulFlagsLockHW;
	unsigned long handle = 0;
	long ret = 0;
	struct VAL_TIME_T rCurTime;
	unsigned int u4TimeInterval;
	enum VAL_RESULT_T eValRet = VAL_RESULT_NO_ERROR;
#if USE_WAKELOCK == 0
	unsigned int suspend_block_cnt = 0;
#endif

	while ((*bLockedHW) == VAL_FALSE) {
		mutex_lock(&HWLockEventTimeoutLock);
		if (HWLockEvent.u4TimeoutMs == 1) {
			pr_info("VDEC_LOCKHW, First Use Dec HW!!\n");
			FirstUseDecHW = 1;
		} else {
			FirstUseDecHW = 0;
		}
		mutex_unlock(&HWLockEventTimeoutLock);

		if (FirstUseDecHW == 1) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			eValRet = eVideoWaitEvent(&HWLockEvent,
						sizeof(struct VAL_EVENT_T));
		}
		mutex_lock(&HWLockEventTimeoutLock);
		if (HWLockEvent.u4TimeoutMs != 1000) {
			HWLockEvent.u4TimeoutMs = 1000;
			FirstUseDecHW = 1;
		} else {
			FirstUseDecHW = 0;
		}
		mutex_unlock(&HWLockEventTimeoutLock);

		mutex_lock(&HWLock);
		/* one process try to lock twice */
		handle = (unsigned long)(pHWLock->pvHandle);
		if (CodecHWLock.pvHandle ==
			(void *)pmem_user_v2p_video(handle)) {
			pr_info("[VDEC] Same inst double lock\n");
			pr_info("may timeout!! inst = 0x%lx, TID = %d\n",
				(unsigned long)CodecHWLock.pvHandle,
				current->pid);
		}
		mutex_unlock(&HWLock);

		if (FirstUseDecHW == 0) {
			pr_debug("[VDEC] Not first time use HW, timeout = %d\n",
			HWLockEvent.u4TimeoutMs);
			eValRet = eVideoWaitEvent(&HWLockEvent,
						sizeof(struct VAL_EVENT_T));
		}

		if (eValRet == VAL_RESULT_INVALID_ISR) {
			ret = vcodec_lockhw_dec_fail(*pHWLock, FirstUseDecHW);
			if (ret) {
				pr_info("[VDEC] lockhw_dec_fail: %lu\n",
						ret);
				return -EFAULT;
			}
		} else if (eValRet == VAL_RESULT_RESTARTSYS) {
			pr_info("[VDEC] ERESTARTSYS when LockHW\n");
			return -ERESTARTSYS;
		}

		mutex_lock(&HWLock);
		if (CodecHWLock.pvHandle == 0) {
			/* No one holds dec hw lock now */
#if USE_WAKELOCK == 1
			__pm_stay_awake(&v_wakeup_src);
#elif USE_WAKELOCK == 0
			while (is_entering_suspend == 1) {
				suspend_block_cnt++;
				if (suspend_block_cnt > 100000) {
					/* Print log if trying to enter suspend
					 * for too long
					 */
					pr_info("VDEC blocked by suspend");
					suspend_block_cnt = 0;
				}
				msleep(1);
			}
#endif
			gu4VdecLockThreadId = current->pid;
			CodecHWLock.pvHandle =
				(void *)pmem_user_v2p_video(handle);
			CodecHWLock.eDriverType = pHWLock->eDriverType;
			eVideoGetTimeOfDay(&CodecHWLock.rLockedTime,
					sizeof(struct VAL_TIME_T));

			pr_debug("VDEC_LOCKHW, free to use HW\n");
			pr_debug("Inst = 0x%lx TID = %d, Time(s, us) = %d, %d",
					handle,
					current->pid,
					CodecHWLock.rLockedTime.u4Sec,
					CodecHWLock.rLockedTime.u4uSec);

			*bLockedHW = VAL_TRUE;
			if (eValRet == VAL_RESULT_INVALID_ISR &&
				FirstUseDecHW != 1) {
				pr_info("[WARN] reset pwr/irq when HWLock");
#ifndef KS_POWER_WORKAROUND
				vdec_power_off();
#endif
				disable_irq(VDEC_IRQ_ID);
			}
#ifndef KS_POWER_WORKAROUND
			vdec_power_on();
#endif
			if (pHWLock->bSecureInst == VAL_FALSE) {
				/* Add one line comment for avoid kernel coding
				 * style, WARNING:BRACES:
				 */
				enable_irq(VDEC_IRQ_ID);
			}
		} else { /* Another one holding dec hw now */
			pr_info("VDEC_LOCKHW E\n");
			eVideoGetTimeOfDay(&rCurTime,
						sizeof(struct VAL_TIME_T));
			u4TimeInterval = (((((rCurTime.u4Sec -
					CodecHWLock.rLockedTime.u4Sec) *
					1000000) + rCurTime.u4uSec) -
					CodecHWLock.rLockedTime.u4uSec) /
					1000);

			pr_debug("VDEC_LOCKHW, someone use HW\n");
			pr_debug("Time(ms) = %d, TimeOut(ms)) = %d\n",
					u4TimeInterval, pHWLock->u4TimeoutMs);
			pr_debug("Lock Inst = 0x%lx, TID = %d, CurrTID = %d\n",
				(unsigned long)CodecHWLock.pvHandle,
				gu4VdecLockThreadId,
				current->pid);
			pr_debug("rLockedTime(%ds, %dus), rCurTime(%ds, %dus)",
					CodecHWLock.rLockedTime.u4Sec,
					CodecHWLock.rLockedTime.u4uSec,
					rCurTime.u4Sec, rCurTime.u4uSec);

			/* 2012/12/16. Cheng-Jung Never steal hardware lock */
		}
		mutex_unlock(&HWLock);
		spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
		gu4LockDecHWCount++;
		spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);
	}
	return 0;
}

static long vcodec_lockhw_venc(struct VAL_HW_LOCK_T *pHWLock, char *bLockedHW)
{
	unsigned int FirstUseEncHW = 0;
	unsigned long handle = 0;
	long ret = 0;
	struct VAL_TIME_T rCurTime;
	unsigned int u4TimeInterval;
	enum VAL_RESULT_T eValRet = VAL_RESULT_NO_ERROR;
#if USE_WAKELOCK == 0
	unsigned int suspend_block_cnt = 0;
#endif

	while ((*bLockedHW) == VAL_FALSE) {
		/* Early break for JPEG VENC */
		if (pHWLock->u4TimeoutMs == 0) {
			if (CodecHWLock.pvHandle != 0) {
				/* Add one line comment for avoid
				 * kernel coding style, WARNING:BRACES:
				 */
				break;
			}
		}

		/* Wait to acquire Enc HW lock */
		mutex_lock(&HWLockEventTimeoutLock);
		if (HWLockEvent.u4TimeoutMs == 1) {
			pr_info("VENC_LOCKHW, First Use Enc HW %d!!\n",
					pHWLock->eDriverType);
			FirstUseEncHW = 1;
		} else {
			FirstUseEncHW = 0;
		}
		mutex_unlock(&HWLockEventTimeoutLock);
		if (FirstUseEncHW == 1) {
			/* Add one line comment for avoid kernel coding
			 * style, WARNING:BRACES:
			 */
			eValRet = eVideoWaitEvent(&HWLockEvent,
						sizeof(struct VAL_EVENT_T));
		}

		mutex_lock(&HWLockEventTimeoutLock);
		if (HWLockEvent.u4TimeoutMs == 1) {
			HWLockEvent.u4TimeoutMs = 1000;
			FirstUseEncHW = 1;
		} else {
			FirstUseEncHW = 0;
			if (pHWLock->u4TimeoutMs == 0) {
				/* Add one line comment for avoid kernel coding
				 * style, WARNING:BRACES:
				 */
				/* No wait */
				HWLockEvent.u4TimeoutMs = 0;
			} else {
				/* Wait indefinitely */
				HWLockEvent.u4TimeoutMs = 1000;
			}
		}
		mutex_unlock(&HWLockEventTimeoutLock);

		mutex_lock(&HWLock);
		handle = (unsigned long)(pHWLock->pvHandle);
		/* one process try to lock twice */
		if (CodecHWLock.pvHandle ==
			(void *)pmem_user_v2p_video(handle)) {
			pr_info("VENC_LOCKHW, Some inst double lock");
			pr_info("may timeout inst=0x%lx, TID=%d, type:%d\n",
				(unsigned long)CodecHWLock.pvHandle,
				current->pid, pHWLock->eDriverType);
		}
		mutex_unlock(&HWLock);

		if (FirstUseEncHW == 0) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			eValRet = eVideoWaitEvent(&HWLockEvent,
						sizeof(struct VAL_EVENT_T));
		}

		if (eValRet == VAL_RESULT_INVALID_ISR) {
			ret = vcodec_lockhw_enc_fail(*pHWLock, FirstUseEncHW);
			if (ret) {
				pr_info("lockhw_enc_fail: %lu\n",
					ret);
				return -EFAULT;
			}
		} else if (eValRet == VAL_RESULT_RESTARTSYS) {
			return -ERESTARTSYS;
		}

		mutex_lock(&HWLock);
		/* No process use HW, so current process can use HW */
		if (CodecHWLock.pvHandle == 0) {
			if (pHWLock->eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
				pHWLock->eDriverType ==
					VAL_DRIVER_TYPE_JPEG_ENC) {
#if USE_WAKELOCK == 1
				__pm_stay_awake(&v_wakeup_src);
#elif USE_WAKELOCK == 0
				while (is_entering_suspend == 1) {
					suspend_block_cnt++;
					if (suspend_block_cnt > 100000) {
						/* Print log if trying to enter
						 * suspend for too long
						 */
					pr_info("VENC blocked by suspend");
						suspend_block_cnt = 0;
					}
					msleep(1);
				}
#endif
				CodecHWLock.pvHandle =
				(void *)pmem_user_v2p_video(handle);
				CodecHWLock.eDriverType = pHWLock->eDriverType;
				eVideoGetTimeOfDay(
					&CodecHWLock.rLockedTime,
					sizeof(struct VAL_TIME_T));

				pr_debug("VENC_LOCKHW, free to use HW\n");
				pr_debug("VENC_LOCKHW, handle = 0x%lx\n",
				(unsigned long)CodecHWLock.pvHandle);
				pr_debug("Inst=0x%lx TID=%d, Time(%ds, %dus)",
				(unsigned long)CodecHWLock.pvHandle,
				current->pid,
				CodecHWLock.rLockedTime.u4Sec,
				CodecHWLock.rLockedTime.u4uSec);

				*bLockedHW = VAL_TRUE;
				if (pHWLock->eDriverType ==
					VAL_DRIVER_TYPE_H264_ENC ||
					pHWLock->eDriverType ==
					VAL_DRIVER_TYPE_JPEG_ENC) {
#ifndef KS_POWER_WORKAROUND
					venc_power_on();
#endif
					enable_irq(VENC_IRQ_ID);
				}
			}
		} else { /* someone use HW, and check timeout value */
			if (pHWLock->u4TimeoutMs == 0) {
				*bLockedHW = VAL_FALSE;
				mutex_unlock(&HWLock);
				break;
			}

			eVideoGetTimeOfDay(&rCurTime,
						sizeof(struct VAL_TIME_T));
			u4TimeInterval = (((((rCurTime.u4Sec -
				CodecHWLock.rLockedTime.u4Sec) *
				1000000) + rCurTime.u4uSec) -
				CodecHWLock.rLockedTime.u4uSec) / 1000);

			pr_debug("VENC_LOCKHW, someone use enc HW");
			pr_debug("Time(ms) = %d, TimeOut(ms) = %d\n",
				 u4TimeInterval, pHWLock->u4TimeoutMs);
			pr_debug("rLockedTime(%ds, %dus), rCurTime(%ds, %dus)",
				CodecHWLock.rLockedTime.u4Sec,
				CodecHWLock.rLockedTime.u4uSec,
				rCurTime.u4Sec, rCurTime.u4uSec);
			pr_debug("LockInst = 0x%lx, Inst = 0x%lx, TID = %d\n",
				 (unsigned long)CodecHWLock.pvHandle,
				 pmem_user_v2p_video(handle),
				 current->pid);

			++gLockTimeOutCount;
			if (gLockTimeOutCount > 30) {
				pr_info("VENC_LOCKHW %d fail 30 times",
						current->pid);
				pr_info("no TO 0x%lx,%lx,0x%lx,type:%d\n",
				(unsigned long)CodecHWLock.pvHandle,
				pmem_user_v2p_video(handle),
				handle,
				pHWLock->eDriverType);
				gLockTimeOutCount = 0;
				mutex_unlock(&HWLock);
				return -EFAULT;
			}

			/* 2013/04/10. Cheng-Jung Never steal hardware lock */
		}

		if (*bLockedHW == VAL_TRUE) {
			pr_debug("VENC_LOCKHW ok handle:0x%lx,va:%lx,type:%d\n",
				(unsigned long)CodecHWLock.pvHandle,
				(unsigned long)(pHWLock->pvHandle),
				pHWLock->eDriverType);
				gLockTimeOutCount = 0;
		}
		mutex_unlock(&HWLock);
	}
	return 0;
}


#define ENC_LOCK_LOG \
	"VENC_LOCKHW \%d failed, locker 0x\%lx,\%lx,0x\%lx,type:\%d"

static long vcodec_lockhw(unsigned long arg)
{
	unsigned char *user_data_addr;
	struct VAL_HW_LOCK_T rHWLock;
	unsigned long handle;
	enum VAL_RESULT_T eValRet;
	long ret;
	char bLockedHW = VAL_FALSE;
	unsigned long ulFlagsLockHW;
	unsigned int u4VcodecSel;
	unsigned int u4DeBlocking = 1;

	pr_debug("VCODEC_LOCKHW + tid = %d\n", current->pid);

	user_data_addr = (unsigned char *)arg;
	ret = copy_from_user(&rHWLock, user_data_addr, sizeof(struct
							VAL_HW_LOCK_T));
	if (ret) {
		pr_info("[VCODEC] LOCKHW copy_from_user failed: %lu\n",
				ret);
		return -EFAULT;
	}

	pr_debug("[VCODEC] LOCKHW eDriverType = %d\n",
			rHWLock.eDriverType);
	eValRet = VAL_RESULT_INVALID_ISR;
	if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_HEVC_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC) {
		return vcodec_lockhw_vdec(&rHWLock, &bLockedHW);
	} else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
			(rHWLock.eDriverType == VAL_DRIVER_TYPE_JPEG_ENC)) {
		ret = vcodec_lockhw_venc(&rHWLock, &bLockedHW);
		if (ret != 0) {
			/* If there is error, return immediately */
			return ret;
		}

		if (bLockedHW == VAL_FALSE) {
			handle = (unsigned long)rHWLock.pvHandle;
			pr_info(ENC_LOCK_LOG,
				current->pid,
				(unsigned long)CodecHWLock.pvHandle,
				pmem_user_v2p_video(handle),
				handle,
				rHWLock.eDriverType);
				gLockTimeOutCount = 0;
			return -EFAULT;
		}

		spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
		gu4LockEncHWCount++;
		spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);

		pr_debug("VCODEC_LOCKHW, get locked - ObjId =%d\n",
				current->pid);

		pr_debug("VCODEC_LOCKHWed - tid = %d\n", current->pid);
	} else {
		pr_info("[WARNING] VCODEC_LOCKHW Unknown instance\n");
		return -EFAULT;
	}

	/* shared vdec/venc VCODEC_SEL setting */
	if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_HEVC_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC) {
		u4VcodecSel = 0x2;
	} else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC) {
		u4VcodecSel = 0x1;
		if (VDO_HW_READ(KVA_VDEC_GCON_BASE + 0x24) == 0) {
			do {
				VDO_HW_WRITE(KVA_VDEC_GCON_BASE + 0x24,
						u4DeBlocking);
			} while (VDO_HW_READ(KVA_VDEC_GCON_BASE + 0x24)
					!= u4DeBlocking);
		}
	} else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_JPEG_ENC) {
		u4VcodecSel = 0x4;
	} else {
		u4VcodecSel = 0x0;
		pr_info("[WARNING] Unknown driver type\n");
	}

	if (VDO_HW_READ(KVA_VDEC_GCON_BASE + 0x20) == 0) {
		do {
			VDO_HW_WRITE(KVA_VDEC_GCON_BASE + 0x20, u4VcodecSel);
		} while (VDO_HW_READ(KVA_VDEC_GCON_BASE + 0x20) != u4VcodecSel);
	} else {
		pr_info("[WARNING] VCODEC_SEL is not 0\n");
	}
	if (u4VcodecSel == 0x2)
		VDO_HW_WRITE(KVA_VDEC_GCON_BASE + 0x10, 0x1);
	else
		VDO_HW_WRITE(KVA_VDEC_GCON_BASE + 0x10, 0x0);

	pr_debug("VCODEC_LOCKHW - tid = %d\n", current->pid);

	return 0;
}

static long vcodec_unlockhw(unsigned long arg)
{
	unsigned char *user_data_addr;
	struct VAL_HW_LOCK_T rHWLock;
	unsigned long handle = 0;
	enum VAL_RESULT_T eValRet;
	long ret;

	pr_debug("VCODEC_UNLOCKHW + tid = %d\n", current->pid);

	user_data_addr = (unsigned char *)arg;
	ret = copy_from_user(&rHWLock, user_data_addr,
				sizeof(struct VAL_HW_LOCK_T));
	if (ret) {
		pr_info("[VCODEC] UNLOCKHW, copy_from_user failed: %lu\n",
				ret);
		return -EFAULT;
	}

	pr_debug("VCODEC_UNLOCKHW eDriverType = %d\n",
			rHWLock.eDriverType);
	eValRet = VAL_RESULT_INVALID_ISR;
	handle = (unsigned long)rHWLock.pvHandle;
	if (rHWLock.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_HEVC_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
		rHWLock.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC) {
		mutex_lock(&HWLock);
		/* Current owner give up hw lock */
		if (CodecHWLock.pvHandle ==
				(void *)pmem_user_v2p_video(handle)) {
			if (rHWLock.bSecureInst == VAL_FALSE) {
				/* Add one line comment for avoid kernel coding
				 * style, WARNING:BRACES:
				 */
				disable_irq(VDEC_IRQ_ID);
			}
			/* TODO: check if turning power off is ok */
#ifndef KS_POWER_WORKAROUND
			vdec_power_off();
#endif
			CodecHWLock.pvHandle = 0;
			CodecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
		} else { /* Not current owner */
			pr_info("[ERROR] VDEC_UNLOCKHW\n");
			pr_info("VDEC Not owner trying to unlock 0x%lx\n",
					pmem_user_v2p_video(handle));
			mutex_unlock(&HWLock);
			return -EFAULT;
		}
#if USE_WAKELOCK == 1
		__pm_relax(&v_wakeup_src);
#endif
		mutex_unlock(&HWLock);
		eValRet = eVideoSetEvent(&HWLockEvent,
					sizeof(struct VAL_EVENT_T));
	} else if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
			 rHWLock.eDriverType == VAL_DRIVER_TYPE_JPEG_ENC) {
		mutex_lock(&HWLock);
		/* Current owner give up hw lock */
		if (CodecHWLock.pvHandle ==
				(void *)pmem_user_v2p_video(handle)) {
			if (rHWLock.eDriverType == VAL_DRIVER_TYPE_H264_ENC ||
				rHWLock.eDriverType ==
					VAL_DRIVER_TYPE_JPEG_ENC) {
				disable_irq(VENC_IRQ_ID);
				/* turn venc power off */
#ifndef KS_POWER_WORKAROUND
				venc_power_off();
#endif
			}
			CodecHWLock.pvHandle = 0;
			CodecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
		} else { /* Not current owner */
			/* [TODO] error handling */
			pr_info("[ERROR] VENC_UNLOCKHW\n");
			pr_info("VENC Not owner trying to unlock 0x%lx\n",
				pmem_user_v2p_video(handle));
			mutex_unlock(&HWLock);
			return -EFAULT;
			}
#if USE_WAKELOCK == 1
		__pm_relax(&v_wakeup_src);
#endif
		mutex_unlock(&HWLock);
		eValRet = eVideoSetEvent(&HWLockEvent,
					sizeof(struct VAL_EVENT_T));
	} else {
		pr_info("[WARNING] VCODEC_UNLOCKHW Unknown instance\n");
		return -EFAULT;
	}

	pr_debug("VCODEC_UNLOCKHW - tid = %d\n", current->pid);

	return 0;
}

static long vcodec_waitisr(unsigned long arg)
{
	unsigned char *user_data_addr;
	struct VAL_ISR_T val_isr;
	char bLockedHW = VAL_FALSE;
	unsigned long ulFlags;
	unsigned long handle;
	long ret;
	enum VAL_RESULT_T eValRet;

	pr_debug("VCODEC_WAITISR + tid = %d\n", current->pid);

	user_data_addr = (unsigned char *)arg;
	ret = copy_from_user(&val_isr, user_data_addr,
				sizeof(struct VAL_ISR_T));
	if (ret) {
		pr_info("[VCODEC] WAITISR, copy_from_user failed: %lu\n",
				ret);
		return -EFAULT;
	}

	handle = (unsigned long)val_isr.pvHandle;
	if (val_isr.eDriverType == VAL_DRIVER_TYPE_MP4_DEC ||
		val_isr.eDriverType == VAL_DRIVER_TYPE_HEVC_DEC ||
		val_isr.eDriverType == VAL_DRIVER_TYPE_H264_DEC ||
		val_isr.eDriverType == VAL_DRIVER_TYPE_MP1_MP2_DEC) {
		mutex_lock(&HWLock);
		if (CodecHWLock.pvHandle ==
				(void *)pmem_user_v2p_video(handle)) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			bLockedHW = VAL_TRUE;
		} else {
		}
		mutex_unlock(&HWLock);

		if (bLockedHW == VAL_FALSE) {
			pr_info("VDEC_WAITISR, DO NOT have HWLock, fail\n");
			return -EFAULT;
		}

		spin_lock_irqsave(&DecIsrLock, ulFlags);
		DecIsrEvent.u4TimeoutMs = val_isr.u4TimeoutMs;
		spin_unlock_irqrestore(&DecIsrLock, ulFlags);

		eValRet = eVideoWaitEvent(&DecIsrEvent,
					sizeof(struct VAL_EVENT_T));
		if (eValRet == VAL_RESULT_INVALID_ISR) {
			return -2;
		} else if (eValRet == VAL_RESULT_RESTARTSYS) {
			pr_info("VDEC_WAITISR, ERESTARTSYS\n");
			return -ERESTARTSYS;
		}
	} else if (val_isr.eDriverType == VAL_DRIVER_TYPE_H264_ENC) {
		mutex_lock(&HWLock);
		if (CodecHWLock.pvHandle ==
				(void *)pmem_user_v2p_video(handle)) {
			/* Add one line comment for avoid kernel coding style,
			 * WARNING:BRACES:
			 */
			bLockedHW = VAL_TRUE;
		} else {
		}
		mutex_unlock(&HWLock);

		if (bLockedHW == VAL_FALSE) {
			pr_info("VENC_WAITISR, DO NOT have HWLock, fail\n");
			return -EFAULT;
		}

		spin_lock_irqsave(&EncIsrLock, ulFlags);
		EncIsrEvent.u4TimeoutMs = val_isr.u4TimeoutMs;
		spin_unlock_irqrestore(&EncIsrLock, ulFlags);

		eValRet = eVideoWaitEvent(&EncIsrEvent,
					sizeof(struct VAL_EVENT_T));
		if (eValRet == VAL_RESULT_INVALID_ISR) {
			return -2;
		} else if (eValRet == VAL_RESULT_RESTARTSYS) {
			pr_info("VENC_WAITISR, ERESTARTSYS\n");
			return -ERESTARTSYS;
		}

		if (val_isr.u4IrqStatusNum > 0) {
			val_isr.u4IrqStatus[0] = gu4HwVencIrqStatus;
			ret = copy_to_user(user_data_addr, &val_isr,
					sizeof(struct VAL_ISR_T));
			if (ret) {
				pr_info("VENC_WAITISR, cp status fail: %lu",
						ret);
				return -EFAULT;
			}
		}
	} else {
		pr_info("[WARNING] VCODEC_WAITISR Unknown instance\n");
		return -EFAULT;
	}

	pr_debug("VCODEC_WAITISR - tid = %d\n", current->pid);

	return 0;
}

static long vcodec_unlocked_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long ret;
	unsigned char *user_data_addr;
	int temp_nr_cpu_ids;
	char rIncLogCount;

#if 0
	VCODEC_DRV_CMD_QUEUE_T rDrvCmdQueue;
	P_VCODEC_DRV_CMD_T cmd_queue = VAL_NULL;
	unsigned int u4Size, uValue, nCount;
#endif

	switch (cmd) {
	case VCODEC_SET_THREAD_ID:
	{
		pr_info("[INFO] VCODEC_SET_THREAD_ID empty");
	}
	break;

	case VCODEC_ALLOC_NON_CACHE_BUFFER:
	{
		pr_info("[INFO] VCODEC_ALLOC_NON_CACHE_BUFFER  empty");
	}
	break;

	case VCODEC_FREE_NON_CACHE_BUFFER:
	{
		pr_info("[INFO] VCODEC_FREE_NON_CACHE_BUFFER  empty");
	}
	break;

	case VCODEC_INC_DEC_EMI_USER:
	{
		pr_debug("VCODEC_INC_DEC_EMI_USER + tid = %d\n",
				current->pid);

		mutex_lock(&DecEMILock);
		gu4DecEMICounter++;
		pr_debug("[VCODEC] DEC_EMI_USER = %d\n",
				gu4DecEMICounter);
		user_data_addr = (unsigned char *)arg;
		ret = copy_to_user(user_data_addr, &gu4DecEMICounter,
				sizeof(unsigned int));
		if (ret) {
			pr_info("INC_DEC_EMI_USER, copy_to_user fail: %lu",
					ret);
			mutex_unlock(&DecEMILock);
			return -EFAULT;
		}
		mutex_unlock(&DecEMILock);

		pr_debug("VCODEC_INC_DEC_EMI_USER - tid = %d\n",
				current->pid);
	}
	break;

	case VCODEC_DEC_DEC_EMI_USER:
	{
		pr_debug("VCODEC_DEC_DEC_EMI_USER + tid = %d\n",
				current->pid);

		mutex_lock(&DecEMILock);
		gu4DecEMICounter--;
		pr_debug("[VCODEC] DEC_EMI_USER = %d\n",
				gu4DecEMICounter);
		user_data_addr = (unsigned char *)arg;
		ret = copy_to_user(user_data_addr, &gu4DecEMICounter,
				sizeof(unsigned int));
		if (ret) {
			pr_info("DEC_DEC_EMI_USER, copy_to_user fail: %lu",
					ret);
			mutex_unlock(&DecEMILock);
			return -EFAULT;
		}
		mutex_unlock(&DecEMILock);

		pr_debug("VCODEC_DEC_DEC_EMI_USER - tid = %d\n",
				current->pid);
	}
	break;

	case VCODEC_INC_ENC_EMI_USER:
	{
		pr_debug("VCODEC_INC_ENC_EMI_USER + tid = %d\n",
				current->pid);

		mutex_lock(&EncEMILock);
		gu4EncEMICounter++;
		pr_info("[VCODEC] ENC_EMI_USER = %d\n",
				gu4EncEMICounter);
		user_data_addr = (unsigned char *)arg;
		ret = copy_to_user(user_data_addr, &gu4EncEMICounter,
				sizeof(unsigned int));
		if (ret) {
			pr_info("INC_ENC_EMI_USER, copy_to_user fail: %lu",
					ret);
			mutex_unlock(&EncEMILock);
			return -EFAULT;
		}
		mutex_unlock(&EncEMILock);

		pr_debug("VCODEC_INC_ENC_EMI_USER - tid = %d\n",
				current->pid);
	}
	break;

	case VCODEC_DEC_ENC_EMI_USER:
	{
		pr_debug("VCODEC_DEC_ENC_EMI_USER + tid = %d\n",
				current->pid);

		mutex_lock(&EncEMILock);
		gu4EncEMICounter--;
		pr_info("[VCODEC] ENC_EMI_USER = %d\n",
				gu4EncEMICounter);
		user_data_addr = (unsigned char *)arg;
		ret = copy_to_user(user_data_addr, &gu4EncEMICounter,
					sizeof(unsigned int));
		if (ret) {
			pr_info("DEC_ENC_EMI_USER, copy_to_user fail: %lu",
					ret);
			mutex_unlock(&EncEMILock);
			return -EFAULT;
		}
		mutex_unlock(&EncEMILock);

		pr_debug("VCODEC_DEC_ENC_EMI_USER - tid = %d\n",
				current->pid);
	}
	break;

	case VCODEC_LOCKHW:
	{
		ret = vcodec_lockhw(arg);
		if (ret) {
			pr_info("[ERROR] VCODEC_LOCKHW failed! %lu\n",
					ret);
			return ret;
		}
	}
		break;

	case VCODEC_UNLOCKHW:
	{
		ret = vcodec_unlockhw(arg);
		if (ret) {
			pr_info("[ERROR] VCODEC_UNLOCKHW failed! %lu\n",
					ret);
			return ret;
		}
	}
		break;

	case VCODEC_INC_PWR_USER:
	{
		pr_info("[INFO] VCODEC_INC_PWR_USER empty");
	}
	break;

	case VCODEC_DEC_PWR_USER:
	{
		pr_info("[INFO] VCODEC_DEC_PWR_USER empty");
	}
	break;

	case VCODEC_WAITISR:
	{
		ret = vcodec_waitisr(arg);
		if (ret) {
			pr_info("[ERROR] VCODEC_WAITISR failed! %lu\n",
					ret);
			return ret;
		}
	}
	break;

	case VCODEC_INITHWLOCK:
	{
		pr_info("[INFO] VCODEC_INITHWLOCK empty");
	}
	break;

	case VCODEC_DEINITHWLOCK:
	{
		pr_info("[INFO] VCODEC_DEINITHWLOCK empty");
	}
	break;

	case VCODEC_GET_CPU_LOADING_INFO:
	{
		pr_info("[INFO] VCODEC_GET_CPU_LOADING empty");
	}
	break;

	case VCODEC_GET_CORE_LOADING:
	{
		pr_info("[INFO] VCODEC_GET_CORE_LOADING empty");
	}
	break;

	case VCODEC_GET_CORE_NUMBER:
	{
		pr_debug("VCODEC_GET_CORE_NUMBER + - tid = %d\n",
				current->pid);

		user_data_addr = (unsigned char *)arg;
		temp_nr_cpu_ids = nr_cpu_ids;
		ret = copy_to_user(user_data_addr, &temp_nr_cpu_ids,
				sizeof(int));
		if (ret) {
			pr_info("GET_CORE_NUMBER, copy_to_user failed: %lu",
					ret);
			return -EFAULT;
		}
		pr_debug("VCODEC_GET_CORE_NUMBER - - tid = %d\n",
				current->pid);
	}
	break;

	case VCODEC_SET_CPU_OPP_LIMIT:
	{
		pr_info("[INFO] VCODEC_SET_CPU_OPP_LIMIT empty");
	}
	break;

	case VCODEC_MB:
	{
		/* For userspace to guarantee register setting order */
		mb();
	}
	break;

	case VCODEC_SET_LOG_COUNT:
	{
		pr_debug("VCODEC_SET_LOG_COUNT + tid = %d\n",
				current->pid);

		mutex_lock(&LogCountLock);
		user_data_addr = (unsigned char *)arg;
		ret = copy_from_user(&rIncLogCount, user_data_addr,
				sizeof(char));
		if (ret) {
			pr_info("SET_LOG_COUNT, copy_from_user failed: %lu",
					ret);
			mutex_unlock(&LogCountLock);
			return -EFAULT;
		}

/* TODO: Log setting API not available in kernel-4.9 yet */
#if 0
		if (rIncLogCount == VAL_TRUE) {
			if (gu4LogCountUser == 0) {
				gu4LogCount = get_detect_count();
				set_detect_count(gu4LogCount + 100);
			}
			gu4LogCountUser++;
		} else {
			gu4LogCountUser--;
			if (gu4LogCountUser == 0) {
				set_detect_count(gu4LogCount);
				gu4LogCount = 0;
			}
		}
#endif
		mutex_unlock(&LogCountLock);

		pr_debug("VCODEC_SET_LOG_COUNT - tid = %d\n",
				current->pid);
	}
	break;

	default:
	{
		pr_info("[ERROR] vcodec_ioctl default case %u\n", cmd);
	}
	break;

	}
	return 0xFF;
}

#if IS_ENABLED(CONFIG_COMPAT)

enum STRUCT_TYPE {
	VAL_HW_LOCK_TYPE = 0,
	VAL_POWER_TYPE,
	VAL_ISR_TYPE,
	VAL_MEMORY_TYPE
};

enum COPY_DIRECTION {
	COPY_FROM_USER = 0,
	COPY_TO_USER,
};

struct COMPAT_VAL_HW_LOCK_T {
	/* [IN]     The video codec driver handle */
	compat_uptr_t       pvHandle;
	/* [IN]     The size of video codec driver handle */
	compat_uint_t       u4HandleSize;
	/* [IN/OUT] The Lock discriptor */
	compat_uptr_t       pvLock;
	/* [IN]     The timeout ms */
	compat_uint_t       u4TimeoutMs;
	/* [IN/OUT] The reserved parameter */
	compat_uptr_t       pvReserved;
	/* [IN]     The size of reserved parameter structure */
	compat_uint_t       u4ReservedSize;
	/* [IN]     The driver type */
	compat_uint_t       eDriverType;
	/* [IN]     True if this is a secure instance */
	/* // MTK_SEC_VIDEO_PATH_SUPPORT */
	char                bSecureInst;
};

struct COMPAT_VAL_POWER_T {
	/* [IN]     The video codec driver handle */
	compat_uptr_t       pvHandle;
	/* [IN]     The size of video codec driver handle */
	compat_uint_t       u4HandleSize;
	/* [IN]     The driver type */
	compat_uint_t       eDriverType;
	/* [IN]     Enable or not. */
	char                fgEnable;
	/* [IN/OUT] The reserved parameter */
	compat_uptr_t       pvReserved;
	/* [IN]     The size of reserved parameter structure */
	compat_uint_t       u4ReservedSize;
	/* [OUT]    The number of power user right now */
	/* unsigned int        u4L2CUser; */
};

struct COMPAT_VAL_ISR_T {
	/* [IN]     The video codec driver handle */
	compat_uptr_t       pvHandle;
	/* [IN]     The size of video codec driver handle */
	compat_uint_t       u4HandleSize;
	/* [IN]     The driver type */
	compat_uint_t       eDriverType;
	/* [IN]     The isr function */
	compat_uptr_t       pvIsrFunction;
	/* [IN/OUT] The reserved parameter */
	compat_uptr_t       pvReserved;
	/* [IN]     The size of reserved parameter structure */
	compat_uint_t       u4ReservedSize;
	/* [IN]     The timeout in ms */
	compat_uint_t       u4TimeoutMs;
	/* [IN]     The num of return registers when HW done */
	compat_uint_t       u4IrqStatusNum;
	/* [IN/OUT] The value of return registers when HW done */
	compat_uint_t       u4IrqStatus[IRQ_STATUS_MAX_NUM];
};

struct COMPAT_VAL_MEMORY_T {
	/* [IN]     The allocation memory type */
	compat_uint_t       eMemType;
	/* [IN]     The size of memory allocation */
	compat_ulong_t      u4MemSize;
	/* [IN/OUT] The memory virtual address */
	compat_uptr_t       pvMemVa;
	/* [IN/OUT] The memory physical address */
	compat_uptr_t       pvMemPa;
	/* [IN]     The memory byte alignment setting */
	compat_uint_t       eAlignment;
	/* [IN/OUT] The align memory virtual address */
	compat_uptr_t       pvAlignMemVa;
	/* [IN/OUT] The align memory physical address */
	compat_uptr_t       pvAlignMemPa;
	/* [IN]     The memory codec for VENC or VDEC */
	compat_uint_t       eMemCodec;
	compat_uint_t       i4IonShareFd;
	compat_uptr_t       pIonBufhandle;
	/* [IN/OUT] The reserved parameter */
	compat_uptr_t       pvReserved;
	/* [IN]     The size of reserved parameter structure */
	compat_ulong_t      u4ReservedSize;
};

static int get_uptr_to_32(compat_uptr_t *p, void __user **uptr)
{
	void __user *p2p;
	int err = get_user(p2p, uptr);
	*p = ptr_to_compat(p2p);
	return err;
}
static int compat_copy_struct(
			enum STRUCT_TYPE eType,
			enum COPY_DIRECTION eDirection,
			void __user *data32,
			void __user *data)
{
	compat_uint_t u;
	compat_ulong_t l;
	compat_uptr_t p;
	char c;
	int err = 0;

	switch (eType) {
	case VAL_HW_LOCK_TYPE:
	{
		if (eDirection == COPY_FROM_USER) {
			struct COMPAT_VAL_HW_LOCK_T __user *from32 =
					(struct COMPAT_VAL_HW_LOCK_T *)data32;
			struct VAL_HW_LOCK_T __user *to =
						(struct VAL_HW_LOCK_T *)data;

			err = get_user(p, &(from32->pvHandle));
			err |= put_user(compat_ptr(p), &(to->pvHandle));
			err |= get_user(u, &(from32->u4HandleSize));
			err |= put_user(u, &(to->u4HandleSize));
			err |= get_user(p, &(from32->pvLock));
			err |= put_user(compat_ptr(p), &(to->pvLock));
			err |= get_user(u, &(from32->u4TimeoutMs));
			err |= put_user(u, &(to->u4TimeoutMs));
			err |= get_user(p, &(from32->pvReserved));
			err |= put_user(compat_ptr(p), &(to->pvReserved));
			err |= get_user(u, &(from32->u4ReservedSize));
			err |= put_user(u, &(to->u4ReservedSize));
			err |= get_user(u, &(from32->eDriverType));
			err |= put_user(u, &(to->eDriverType));
			err |= get_user(c, &(from32->bSecureInst));
			err |= put_user(c, &(to->bSecureInst));
		} else {
			struct COMPAT_VAL_HW_LOCK_T __user *to32 =
					(struct COMPAT_VAL_HW_LOCK_T *)data32;
			struct VAL_HW_LOCK_T __user *from =
						(struct VAL_HW_LOCK_T *)data;

			err = get_uptr_to_32(&p, &(from->pvHandle));
			err |= put_user(p, &(to32->pvHandle));
			err |= get_user(u, &(from->u4HandleSize));
			err |= put_user(u, &(to32->u4HandleSize));
			err |= get_uptr_to_32(&p, &(from->pvLock));
			err |= put_user(p, &(to32->pvLock));
			err |= get_user(u, &(from->u4TimeoutMs));
			err |= put_user(u, &(to32->u4TimeoutMs));
			err |= get_uptr_to_32(&p, &(from->pvReserved));
			err |= put_user(p, &(to32->pvReserved));
			err |= get_user(u, &(from->u4ReservedSize));
			err |= put_user(u, &(to32->u4ReservedSize));
			err |= get_user(u, &(from->eDriverType));
			err |= put_user(u, &(to32->eDriverType));
			err |= get_user(c, &(from->bSecureInst));
			err |= put_user(c, &(to32->bSecureInst));
		}
	}
	break;
	case VAL_POWER_TYPE:
	{
		if (eDirection == COPY_FROM_USER) {
			struct COMPAT_VAL_POWER_T __user *from32 =
					(struct COMPAT_VAL_POWER_T *)data32;
			struct VAL_POWER_T __user *to =
						(struct VAL_POWER_T *)data;

			err = get_user(p, &(from32->pvHandle));
			err |= put_user(compat_ptr(p), &(to->pvHandle));
			err |= get_user(u, &(from32->u4HandleSize));
			err |= put_user(u, &(to->u4HandleSize));
			err |= get_user(u, &(from32->eDriverType));
			err |= put_user(u, &(to->eDriverType));
			err |= get_user(c, &(from32->fgEnable));
			err |= put_user(c, &(to->fgEnable));
			err |= get_user(p, &(from32->pvReserved));
			err |= put_user(compat_ptr(p), &(to->pvReserved));
			err |= get_user(u, &(from32->u4ReservedSize));
			err |= put_user(u, &(to->u4ReservedSize));
		} else {
			struct COMPAT_VAL_POWER_T __user *to32 =
					(struct COMPAT_VAL_POWER_T *)data32;
			struct VAL_POWER_T __user *from =
					(struct VAL_POWER_T *)data;

			err = get_uptr_to_32(&p, &(from->pvHandle));
			err |= put_user(p, &(to32->pvHandle));
			err |= get_user(u, &(from->u4HandleSize));
			err |= put_user(u, &(to32->u4HandleSize));
			err |= get_user(u, &(from->eDriverType));
			err |= put_user(u, &(to32->eDriverType));
			err |= get_user(c, &(from->fgEnable));
			err |= put_user(c, &(to32->fgEnable));
			err |= get_uptr_to_32(&p, &(from->pvReserved));
			err |= put_user(p, &(to32->pvReserved));
			err |= get_user(u, &(from->u4ReservedSize));
			err |= put_user(u, &(to32->u4ReservedSize));
		}
	}
	break;
	case VAL_ISR_TYPE:
	{
		int i = 0;

		if (eDirection == COPY_FROM_USER) {
			struct COMPAT_VAL_ISR_T __user *from32 =
					(struct COMPAT_VAL_ISR_T *)data32;
			struct VAL_ISR_T __user *to = (struct VAL_ISR_T *)data;

			err = get_user(p, &(from32->pvHandle));
			err |= put_user(compat_ptr(p), &(to->pvHandle));
			err |= get_user(u, &(from32->u4HandleSize));
			err |= put_user(u, &(to->u4HandleSize));
			err |= get_user(u, &(from32->eDriverType));
			err |= put_user(u, &(to->eDriverType));
			err |= get_user(p, &(from32->pvIsrFunction));
			err |= put_user(compat_ptr(p), &(to->pvIsrFunction));
			err |= get_user(p, &(from32->pvReserved));
			err |= put_user(compat_ptr(p), &(to->pvReserved));
			err |= get_user(u, &(from32->u4ReservedSize));
			err |= put_user(u, &(to->u4ReservedSize));
			err |= get_user(u, &(from32->u4TimeoutMs));
			err |= put_user(u, &(to->u4TimeoutMs));
			err |= get_user(u, &(from32->u4IrqStatusNum));
			err |= put_user(u, &(to->u4IrqStatusNum));
			for (; i < IRQ_STATUS_MAX_NUM; i++) {
				err |= get_user(u, &(from32->u4IrqStatus[i]));
				err |= put_user(u, &(to->u4IrqStatus[i]));
			}
			return err;

		} else {
			struct COMPAT_VAL_ISR_T __user *to32 =
					(struct COMPAT_VAL_ISR_T *)data32;
			struct VAL_ISR_T __user *from =
						(struct VAL_ISR_T *)data;

			err = get_uptr_to_32(&p, &(from->pvHandle));
			err |= put_user(p, &(to32->pvHandle));
			err |= get_user(u, &(from->u4HandleSize));
			err |= put_user(u, &(to32->u4HandleSize));
			err |= get_user(u, &(from->eDriverType));
			err |= put_user(u, &(to32->eDriverType));
			err |= get_uptr_to_32(&p, &(from->pvIsrFunction));
			err |= put_user(p, &(to32->pvIsrFunction));
			err |= get_uptr_to_32(&p, &(from->pvReserved));
			err |= put_user(p, &(to32->pvReserved));
			err |= get_user(u, &(from->u4ReservedSize));
			err |= put_user(u, &(to32->u4ReservedSize));
			err |= get_user(u, &(from->u4TimeoutMs));
			err |= put_user(u, &(to32->u4TimeoutMs));
			err |= get_user(u, &(from->u4IrqStatusNum));
			err |= put_user(u, &(to32->u4IrqStatusNum));
			for (; i < IRQ_STATUS_MAX_NUM; i++) {
				err |= get_user(u, &(from->u4IrqStatus[i]));
				err |= put_user(u, &(to32->u4IrqStatus[i]));
			}
		}
	}
	break;
	case VAL_MEMORY_TYPE:
	{
		if (eDirection == COPY_FROM_USER) {
			struct COMPAT_VAL_MEMORY_T __user *from32 =
					(struct COMPAT_VAL_MEMORY_T *)data32;
			struct VAL_MEMORY_T __user *to =
						(struct VAL_MEMORY_T *)data;

			err = get_user(u, &(from32->eMemType));
			err |= put_user(u, &(to->eMemType));
			err |= get_user(l, &(from32->u4MemSize));
			err |= put_user(l, &(to->u4MemSize));
			err |= get_user(p, &(from32->pvMemVa));
			err |= put_user(compat_ptr(p), &(to->pvMemVa));
			err |= get_user(p, &(from32->pvMemPa));
			err |= put_user(compat_ptr(p), &(to->pvMemPa));
			err |= get_user(u, &(from32->eAlignment));
			err |= put_user(u, &(to->eAlignment));
			err |= get_user(p, &(from32->pvAlignMemVa));
			err |= put_user(compat_ptr(p), &(to->pvAlignMemVa));
			err |= get_user(p, &(from32->pvAlignMemPa));
			err |= put_user(compat_ptr(p), &(to->pvAlignMemPa));
			err |= get_user(u, &(from32->eMemCodec));
			err |= put_user(u, &(to->eMemCodec));
			err |= get_user(u, &(from32->i4IonShareFd));
			err |= put_user(u, &(to->i4IonShareFd));
			err |= get_user(p, &(from32->pIonBufhandle));
			err |= put_user(compat_ptr(p), &(to->pIonBufhandle));
			err |= get_user(p, &(from32->pvReserved));
			err |= put_user(compat_ptr(p), &(to->pvReserved));
			err |= get_user(l, &(from32->u4ReservedSize));
			err |= put_user(l, &(to->u4ReservedSize));
		} else {
			struct COMPAT_VAL_MEMORY_T __user *to32 =
					(struct COMPAT_VAL_MEMORY_T *)data32;

			struct VAL_MEMORY_T __user *from =
					(struct VAL_MEMORY_T *)data;

			err = get_user(u, &(from->eMemType));
			err |= put_user(u, &(to32->eMemType));
			err |= get_user(l, &(from->u4MemSize));
			err |= put_user(l, &(to32->u4MemSize));
			err |= get_uptr_to_32(&p, &(from->pvMemVa));
			err |= put_user(p, &(to32->pvMemVa));
			err |= get_uptr_to_32(&p, &(from->pvMemPa));
			err |= put_user(p, &(to32->pvMemPa));
			err |= get_user(u, &(from->eAlignment));
			err |= put_user(u, &(to32->eAlignment));
			err |= get_uptr_to_32(&p, &(from->pvAlignMemVa));
			err |= put_user(p, &(to32->pvAlignMemVa));
			err |= get_uptr_to_32(&p, &(from->pvAlignMemPa));
			err |= put_user(p, &(to32->pvAlignMemPa));
			err |= get_user(u, &(from->eMemCodec));
			err |= put_user(u, &(to32->eMemCodec));
			err |= get_user(u, &(from->i4IonShareFd));
			err |= put_user(u, &(to32->i4IonShareFd));
			err |= get_uptr_to_32(&p,
					(void __user **)&(from->pIonBufhandle));
			err |= put_user(p, &(to32->pIonBufhandle));
			err |= get_uptr_to_32(&p, &(from->pvReserved));
			err |= put_user(p, &(to32->pvReserved));
			err |= get_user(l, &(from->u4ReservedSize));
			err |= put_user(l, &(to32->u4ReservedSize));
		}
	}
	break;
	default:
	break;
	}

	return err;
}


static long vcodec_unlocked_compat_ioctl(struct file *file, unsigned int cmd,
					unsigned long arg)
{
	long ret = 0;
	/* pr_debug("vcodec_unlocked_compat_ioctl: 0x%x\n", cmd); */
	switch (cmd) {
	case VCODEC_ALLOC_NON_CACHE_BUFFER:
	case VCODEC_FREE_NON_CACHE_BUFFER:
	{
		struct COMPAT_VAL_MEMORY_T __user *data32;
		struct VAL_MEMORY_T __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(struct VAL_MEMORY_T));
		if (data == NULL)
			return -EFAULT;

		err = compat_copy_struct(VAL_MEMORY_TYPE, COPY_FROM_USER,
					(void *)data32, (void *)data);
		if (err)
			return err;

		ret = file->f_op->unlocked_ioctl(file, cmd,
						(unsigned long)data);

		err = compat_copy_struct(VAL_MEMORY_TYPE, COPY_TO_USER,
					(void *)data32, (void *)data);

		if (err)
			return err;
		return ret;
	}
	break;
	case VCODEC_LOCKHW:
	case VCODEC_UNLOCKHW:
	{
		struct COMPAT_VAL_HW_LOCK_T __user *data32;
		struct VAL_HW_LOCK_T __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(struct VAL_HW_LOCK_T));
		if (data == NULL)
			return -EFAULT;

		err = compat_copy_struct(VAL_HW_LOCK_TYPE, COPY_FROM_USER,
					(void *)data32, (void *)data);
		if (err)
			return err;

		ret = file->f_op->unlocked_ioctl(file, cmd,
						(unsigned long)data);

		err = compat_copy_struct(VAL_HW_LOCK_TYPE, COPY_TO_USER,
					(void *)data32, (void *)data);

		if (err)
			return err;
		return ret;
	}
	break;

	case VCODEC_INC_PWR_USER:
	case VCODEC_DEC_PWR_USER:
	{
		struct COMPAT_VAL_POWER_T __user *data32;
		struct VAL_POWER_T __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(struct VAL_POWER_T));
		if (data == NULL)
			return -EFAULT;

		err = compat_copy_struct(VAL_POWER_TYPE, COPY_FROM_USER,
					(void *)data32, (void *)data);

		if (err)
			return err;

		ret = file->f_op->unlocked_ioctl(file, cmd,
						(unsigned long)data);

		err = compat_copy_struct(VAL_POWER_TYPE, COPY_TO_USER,
					(void *)data32, (void *)data);

		if (err)
			return err;
		return ret;
	}
	break;

	case VCODEC_WAITISR:
	{
		struct COMPAT_VAL_ISR_T __user *data32;
		struct VAL_ISR_T __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(struct VAL_ISR_T));
		if (data == NULL)
			return -EFAULT;

		err = compat_copy_struct(VAL_ISR_TYPE, COPY_FROM_USER,
					(void *)data32, (void *)data);
		if (err)
			return err;

		ret = file->f_op->unlocked_ioctl(file, VCODEC_WAITISR,
						(unsigned long)data);

		err = compat_copy_struct(VAL_ISR_TYPE, COPY_TO_USER,
					(void *)data32, (void *)data);

		if (err)
			return err;
		return ret;
	}
	break;

	default:
		return vcodec_unlocked_ioctl(file, cmd, arg);
	}
	return 0;
}
#else
#define vcodec_unlocked_compat_ioctl NULL
#endif
static int vcodec_open(struct inode *inode, struct file *file)
{
	pr_debug("vcodec_open\n");

	mutex_lock(&DriverOpenCountLock);
	Driver_Open_Count++;

	pr_info("vcodec_open pid = %d, Driver_Open_Count %d\n",
			current->pid, Driver_Open_Count);
	mutex_unlock(&DriverOpenCountLock);

	/* TODO: Check upper limit of concurrent users? */

	return 0;
}

static int vcodec_flush(struct file *file, fl_owner_t id)
{
	pr_debug("vcodec_flush, curr_tid =%d\n", current->pid);
	pr_debug("vcodec_flush pid = %d, Driver_Open_Count %d\n",
			current->pid, Driver_Open_Count);

	return 0;
}

static int vcodec_release(struct inode *inode, struct file *file)
{
	unsigned long ulFlagsLockHW, ulFlagsISR;

	/* dump_stack(); */
	pr_debug("vcodec_release, curr_tid =%d\n", current->pid);
	mutex_lock(&DriverOpenCountLock);
	pr_info("vcodec_release pid = %d, Driver_Open_Count %d\n",
			current->pid, Driver_Open_Count);
	Driver_Open_Count--;

	if (Driver_Open_Count == 0) {
		mutex_lock(&HWLock);
		gu4VdecLockThreadId = 0;


		/* check if someone didn't unlockHW */
		if (CodecHWLock.pvHandle != 0) {
			pr_info("err vcodec_release %d, type = %d, 0x%lx\n",
				current->pid,
				CodecHWLock.eDriverType,
				(unsigned long)CodecHWLock.pvHandle);
			pr_info("err VCODEC_SEL 0x%x\n",
				VDO_HW_READ(KVA_VDEC_GCON_BASE + 0x20));

			/* power off */
			if (CodecHWLock.eDriverType ==
				VAL_DRIVER_TYPE_MP4_DEC ||
				CodecHWLock.eDriverType ==
					VAL_DRIVER_TYPE_HEVC_DEC ||
				CodecHWLock.eDriverType ==
					VAL_DRIVER_TYPE_H264_DEC ||
				CodecHWLock.eDriverType ==
					VAL_DRIVER_TYPE_MP1_MP2_DEC) {
				vdec_break();
				vdec_power_off();
			} else if (CodecHWLock.eDriverType ==
					VAL_DRIVER_TYPE_H264_ENC ||
				CodecHWLock.eDriverType ==
					VAL_DRIVER_TYPE_HEVC_ENC) {
				venc_break();
				venc_power_off();
			} else if (CodecHWLock.eDriverType ==
					VAL_DRIVER_TYPE_JPEG_ENC) {
				venc_power_off();
			}
		}

		CodecHWLock.pvHandle = 0;
		CodecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
		CodecHWLock.rLockedTime.u4Sec = 0;
		CodecHWLock.rLockedTime.u4uSec = 0;
		mutex_unlock(&HWLock);

		mutex_lock(&DecEMILock);
		gu4DecEMICounter = 0;
		mutex_unlock(&DecEMILock);

		mutex_lock(&EncEMILock);
		gu4EncEMICounter = 0;
		mutex_unlock(&EncEMILock);

		mutex_lock(&PWRLock);
		gu4PWRCounter = 0;
		mutex_unlock(&PWRLock);

		spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
		gu4LockDecHWCount = 0;
		spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);

		spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
		gu4LockEncHWCount = 0;
		spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);

		spin_lock_irqsave(&DecISRCountLock, ulFlagsISR);
		gu4DecISRCount = 0;
		spin_unlock_irqrestore(&DecISRCountLock, ulFlagsISR);

		spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
		gu4EncISRCount = 0;
		spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);
	}
	mutex_unlock(&DriverOpenCountLock);

	return 0;
}

void vcodec_vma_open(struct vm_area_struct *vma)
{
	pr_debug("vcodec VMA open, virt %lx, phys %lx\n", vma->vm_start,
			vma->vm_pgoff << PAGE_SHIFT);
}

void vcodec_vma_close(struct vm_area_struct *vma)
{
	pr_debug("vcodec VMA close, virt %lx, phys %lx\n", vma->vm_start,
			vma->vm_pgoff << PAGE_SHIFT);
}

static const struct vm_operations_struct vcodec_remap_vm_ops = {
	.open = vcodec_vma_open,
	.close = vcodec_vma_close,
};

static int vcodec_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 1
	unsigned int u4I = 0;
	unsigned long length;
	unsigned long pfn;

	length = vma->vm_end - vma->vm_start;
	pfn = vma->vm_pgoff<<PAGE_SHIFT;

	if (((length > VENC_REGION) || (pfn < VENC_BASE) ||
		(pfn > VENC_BASE+VENC_REGION)) &&
		((length > VDEC_REGION) || (pfn < VDEC_BASE_PHY) ||
			(pfn > VDEC_BASE_PHY+VDEC_REGION)) &&
		((length > HW_REGION) || (pfn < HW_BASE) ||
			(pfn > HW_BASE+HW_REGION)) &&
		((length > INFO_REGION) || (pfn < INFO_BASE) ||
			(pfn > INFO_BASE+INFO_REGION))) {
		unsigned long ulAddr, ulSize;

		for (u4I = 0; u4I < VCODEC_INST_NUM_x_10; u4I++) {
			if ((ncache_mem_list[u4I].ulKVA != -1L) &&
				(ncache_mem_list[u4I].ulKPA != -1L)) {
				ulAddr = ncache_mem_list[u4I].ulKPA;
				ulSize = (ncache_mem_list[u4I].ulSize +
						0x1000 - 1) & ~(0x1000 - 1);
				if ((length == ulSize) && (pfn == ulAddr)) {
					pr_debug("[VCODEC] cache idx %d\n",
							u4I);
					break;
				}
			}
		}

		if (u4I == VCODEC_INST_NUM_x_10) {
			pr_info("mmap region error: Len(0x%lx),pfn(0x%lx)",
				 (unsigned long)length, pfn);
			return -EAGAIN;
		}
	}
#endif
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	pr_debug("mmap vma->start 0x%lx, vma->end 0x%lx, vma->pgoff 0x%lx\n",
			(unsigned long)vma->vm_start,
			(unsigned long)vma->vm_end,
			(unsigned long)vma->vm_pgoff);
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	vma->vm_ops = &vcodec_remap_vm_ops;
	vcodec_vma_open(vma);

	return 0;
}

static const struct file_operations vcodec_fops = {
	.owner      = THIS_MODULE,
	.unlocked_ioctl = vcodec_unlocked_ioctl,
	.open       = vcodec_open,
	.flush      = vcodec_flush,
	.release    = vcodec_release,
	.mmap       = vcodec_mmap,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl   = vcodec_unlocked_compat_ioctl,
#endif

};

#if USE_WAKELOCK == 0
/**
 * Suspsend callbacks after user space processes are frozen
 * Since user space processes are frozen, there is no need and cannot hold same
 * mutex that protects lock owner while checking status.
 * If video codec hardware is still active now, must not aenter suspend.
 **/
static int vcodec_suspend(struct platform_device *pDev, pm_message_t state)
{
	if (CodecHWLock.pvHandle != 0) {
		pr_info("vcodec_suspend fail due to videocodec activity");
		return -EBUSY;
	}
	pr_debug("vcodec_suspend ok");
	return 0;
}

static int vcodec_resume(struct platform_device *pDev)
{
	pr_info("vcodec_resume ok");
	return 0;
}

/**
 * Suspend notifiers before user space processes are frozen.
 * User space driver can still complete decoding/encoding of current frame.
 * Change state to is_entering_suspend to stop further tasks but allow current
 * frame to complete (LOCKHW, WAITISR, UNLOCKHW).
 * Since there is no critical section proection, it is possible for a new task
 * to start after changing to is_entering_suspend state. This case will be
 * handled by suspend callback vcodec_suspend.
 **/
static int vcodec_suspend_notifier(struct notifier_block *nb,
					unsigned long action, void *data)
{
	int wait_cnt = 0;

	pr_info("vcodec_suspend_notifier ok action = %ld", action);
	switch (action) {
	case PM_SUSPEND_PREPARE:
		is_entering_suspend = 1;
		while (CodecHWLock.pvHandle != 0) {
			wait_cnt++;
			if (wait_cnt > 100000) {
				pr_info("vcodec_pm_suspend waiting %p %d",
					CodecHWLock.pvHandle,
					(int)CodecHWLock.eDriverType);
				wait_cnt = 0;
			}
			msleep(1);
		}
		return NOTIFY_OK;
	case PM_POST_SUSPEND:
		is_entering_suspend = 0;
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_DONE;
}
#endif

static int vcodec_probe(struct platform_device *dev)
{
	int ret;

	pr_debug("+vcodec_probe\n");

	mutex_lock(&DecEMILock);
	gu4DecEMICounter = 0;
	mutex_unlock(&DecEMILock);

	mutex_lock(&EncEMILock);
	gu4EncEMICounter = 0;
	mutex_unlock(&EncEMILock);

	mutex_lock(&PWRLock);
	gu4PWRCounter = 0;
	mutex_unlock(&PWRLock);

	ret = register_chrdev_region(vcodec_devno, 1, VCODEC_DEVNAME);
	if (ret) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC] Can't Get Major number\n");
	}

	vcodec_cdev = cdev_alloc();
	vcodec_cdev->owner = THIS_MODULE;
	vcodec_cdev->ops = &vcodec_fops;

	ret = cdev_add(vcodec_cdev, vcodec_devno, 1);
	if (ret) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC] Can't add Vcodec Device\n");
	}

	vcodec_class = class_create(THIS_MODULE, VCODEC_DEVNAME);
	if (IS_ERR(vcodec_class)) {
		ret = PTR_ERR(vcodec_class);
		pr_info("[VCODEC] Unable to create class err = %d ",
				 ret);
		return ret;
	}

	vcodec_device = device_create(vcodec_class, NULL, vcodec_devno, NULL,
					VCODEC_DEVNAME);

	if (request_irq(VDEC_IRQ_ID, (irq_handler_t)video_intr_dlr,
			IRQF_TRIGGER_LOW, VCODEC_DEVNAME, NULL) < 0) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] error to request dec irq\n");
	} else {
		pr_debug("[VCODEC] success to request dec irq: %d\n",
				VDEC_IRQ_ID);
	}

	if (request_irq(VENC_IRQ_ID, (irq_handler_t)video_intr_dlr2,
			IRQF_TRIGGER_LOW, VCODEC_DEVNAME, NULL) < 0) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_debug("[VCODEC][ERROR] error to request enc irq\n");
	} else {
		pr_debug("[VCODEC] success to request enc irq: %d\n",
				VENC_IRQ_ID);
	}

	disable_irq(VDEC_IRQ_ID);
	disable_irq(VENC_IRQ_ID);

#ifndef CONFIG_MTK_SMI_EXT
	clk_MT_CG_SMI_COMMON = devm_clk_get(&dev->dev, "MT_CG_SMI_COMMON");
	if (IS_ERR(clk_MT_CG_SMI_COMMON)) {
		pr_info("[VCODEC] Unable to get MT_CG_SMI_COMMON");
		return PTR_ERR(clk_MT_CG_SMI_COMMON);
	}

	clk_MT_CG_GALS_VDEC2MM = devm_clk_get(&dev->dev, "MT_CG_GALS_VDEC2MM");
	if (IS_ERR(clk_MT_CG_GALS_VDEC2MM)) {
		pr_info("[VCODEC] Unable to get MT_CG_GALS_VDEC2MM");
		return PTR_ERR(clk_MT_CG_GALS_VDEC2MM);
	}

	clk_MT_CG_GALS_VENC2MM = devm_clk_get(&dev->dev, "MT_CG_GALS_VENC2MM");
	if (IS_ERR(clk_MT_CG_GALS_VENC2MM)) {
		pr_info("[VCODEC] Unable to get MT_CG_GALS_VENC2MM");
		return PTR_ERR(clk_MT_CG_GALS_VENC2MM);
	}
#endif

	clk_MT_CG_VDEC = devm_clk_get(&dev->dev, "MT_CG_VDEC");
	if (IS_ERR(clk_MT_CG_VDEC)) {
		pr_info("[VCODEC] Unable to get MT_CG_VDEC");
		return PTR_ERR(clk_MT_CG_VDEC);
	}

	clk_MT_CG_VENC = devm_clk_get(&dev->dev, "MT_CG_VENC");
	if (IS_ERR(clk_MT_CG_VENC)) {
		pr_info("[VCODEC] Unable to get MT_CG_VENC");
		return PTR_ERR(clk_MT_CG_VENC);
	}

	clk_MT_SCP_SYS_VCODEC = devm_clk_get(&dev->dev, "MT_SCP_SYS_VCODEC");
	if (IS_ERR(clk_MT_SCP_SYS_VCODEC)) {
		pr_info("[VCODEC] Unable to get MT_SCP_SYS_VCODEC");
		return PTR_ERR(clk_MT_SCP_SYS_VCODEC);
	}

	clk_MT_SCP_SYS_DISP = devm_clk_get(&dev->dev, "MT_SCP_SYS_DISP");
	if (IS_ERR(clk_MT_SCP_SYS_DISP)) {
		pr_info("[VCODEC] Unable to get MT_SCP_SYS_DISP");
		return PTR_ERR(clk_MT_SCP_SYS_DISP);
	}
#if USE_WAKELOCK == 0
	pm_notifier(vcodec_suspend_notifier, 0);
#endif

	pr_debug("vcodec_probe Done\n");

#ifdef KS_POWER_WORKAROUND
	vdec_power_on();
	venc_power_on();
#endif

	return 0;
}

static int vcodec_remove(struct platform_device *pDev)
{
	pr_debug("vcodec_remove\n");
	return 0;
}

#ifdef CONFIG_MTK_HIBERNATION
/* extern void mt_irq_set_sens(unsigned int irq, unsigned int sens); */
/* extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity); */
static int vcodec_pm_restore_noirq(struct device *device)
{
	/* vdec: IRQF_TRIGGER_LOW */
	mt_irq_set_sens(VDEC_IRQ_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(VDEC_IRQ_ID, MT_POLARITY_LOW);
	/* venc: IRQF_TRIGGER_LOW */
	mt_irq_set_sens(VENC_IRQ_ID, MT_LEVEL_SENSITIVE);
	mt_irq_set_polarity(VENC_IRQ_ID, MT_POLARITY_LOW);

	return 0;
}
#endif

static const struct of_device_id vcodec_of_match[] = {
	{ .compatible = "mediatek,vdec_top_global_con", },
	{/* sentinel */}
};

MODULE_DEVICE_TABLE(of, vcodec_of_match);

static struct platform_driver vcodec_driver = {
	.probe = vcodec_probe,
	.remove = vcodec_remove,
#if USE_WAKELOCK == 0
	.suspend = vcodec_suspend,
	.resume = vcodec_resume,
#endif
	.driver = {
		.name  = VCODEC_DEVNAME,
		.owner = THIS_MODULE,
		.of_match_table = vcodec_of_match,
	},
};

static int __init vcodec_driver_init(void)
{
	enum VAL_RESULT_T  eValHWLockRet;
	unsigned long ulFlags, ulFlagsLockHW, ulFlagsISR;

	pr_debug("+vcodec_driver_init !!\n");

	mutex_lock(&DriverOpenCountLock);
	Driver_Open_Count = 0;
#if USE_WAKELOCK == 1
	wakeup_source_init(&v_wakeup_src, "v_wakeup_src");
#endif
	mutex_unlock(&DriverOpenCountLock);

	mutex_lock(&LogCountLock);
	gu4LogCountUser = 0;
	gu4LogCount = 0;
	mutex_unlock(&LogCountLock);

	{
		struct device_node *node = NULL;

		node = of_find_compatible_node(NULL, NULL, "mediatek,venc");
		KVA_VENC_BASE = of_iomap(node, 0);
		VENC_IRQ_ID =  irq_of_parse_and_map(node, 0);
		KVA_VENC_IRQ_STATUS_ADDR =    KVA_VENC_BASE + 0x05C;
		KVA_VENC_IRQ_ACK_ADDR  = KVA_VENC_BASE + 0x060;
	}

	{
		struct device_node *node = NULL;

		node = of_find_compatible_node(NULL, NULL, "mediatek,vdec");
		KVA_VDEC_BASE = of_iomap(node, 0);
		VDEC_IRQ_ID =  irq_of_parse_and_map(node, 0);
		KVA_VDEC_MISC_BASE = KVA_VDEC_BASE + 0x5000;
		KVA_VDEC_VLD_BASE = KVA_VDEC_BASE + 0x0000;
	}
	{
		struct device_node *node = NULL;

		node = of_find_compatible_node(NULL, NULL,
						"mediatek,venc_gcon");
		KVA_VDEC_GCON_BASE = of_iomap(node, 0);

		pr_debug("[VCODEC] VENC(0x%p), VDEC(0x%p), VDEC_GCON(0x%p)",
			KVA_VENC_BASE, KVA_VDEC_BASE, KVA_VDEC_GCON_BASE);
		pr_debug("[VCODEC] VDEC_IRQ_ID(%d), VENC_IRQ_ID(%d)",
			VDEC_IRQ_ID, VENC_IRQ_ID);
	}

	/* KVA_VENC_IRQ_STATUS_ADDR =
	 *		(unsigned long)ioremap(VENC_IRQ_STATUS_addr, 4);
	 * KVA_VENC_IRQ_ACK_ADDR  =
	 *		(unsigned long)ioremap(VENC_IRQ_ACK_addr, 4);
	 */

	spin_lock_irqsave(&LockDecHWCountLock, ulFlagsLockHW);
	gu4LockDecHWCount = 0;
	spin_unlock_irqrestore(&LockDecHWCountLock, ulFlagsLockHW);

	spin_lock_irqsave(&LockEncHWCountLock, ulFlagsLockHW);
	gu4LockEncHWCount = 0;
	spin_unlock_irqrestore(&LockEncHWCountLock, ulFlagsLockHW);

	spin_lock_irqsave(&DecISRCountLock, ulFlagsISR);
	gu4DecISRCount = 0;
	spin_unlock_irqrestore(&DecISRCountLock, ulFlagsISR);

	spin_lock_irqsave(&EncISRCountLock, ulFlagsISR);
	gu4EncISRCount = 0;
	spin_unlock_irqrestore(&EncISRCountLock, ulFlagsISR);

	mutex_lock(&VdecPWRLock);
	gu4VdecPWRCounter = 0;
	mutex_unlock(&VdecPWRLock);

	mutex_lock(&VencPWRLock);
	gu4VencPWRCounter = 0;
	mutex_unlock(&VencPWRLock);

	mutex_lock(&IsOpenedLock);
	if (bIsOpened == VAL_FALSE) {
		bIsOpened = VAL_TRUE;
		/* vcodec_probe(NULL); */
	}
	mutex_unlock(&IsOpenedLock);

	mutex_lock(&HWLock);
	gu4VdecLockThreadId = 0;
	CodecHWLock.pvHandle = 0;
	CodecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
	CodecHWLock.rLockedTime.u4Sec = 0;
	CodecHWLock.rLockedTime.u4uSec = 0;
	mutex_unlock(&HWLock);

	mutex_lock(&HWLock);
	CodecHWLock.pvHandle = 0;
	CodecHWLock.eDriverType = VAL_DRIVER_TYPE_NONE;
	CodecHWLock.rLockedTime.u4Sec = 0;
	CodecHWLock.rLockedTime.u4uSec = 0;
	mutex_unlock(&HWLock);

	/* HWLockEvent part */
	mutex_lock(&HWLockEventTimeoutLock);
	HWLockEvent.pvHandle = "DECHWLOCK_EVENT";
	HWLockEvent.u4HandleSize = sizeof("DECHWLOCK_EVENT")+1;
	HWLockEvent.u4TimeoutMs = 1;
	mutex_unlock(&HWLockEventTimeoutLock);
	eValHWLockRet = eVideoCreateEvent(&HWLockEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] create dec hwlock event error\n");
	}

	mutex_lock(&HWLockEventTimeoutLock);
	HWLockEvent.pvHandle = "ENCHWLOCK_EVENT";
	HWLockEvent.u4HandleSize = sizeof("ENCHWLOCK_EVENT")+1;
	HWLockEvent.u4TimeoutMs = 1;
	mutex_unlock(&HWLockEventTimeoutLock);
	eValHWLockRet = eVideoCreateEvent(&HWLockEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] create enc hwlock event error\n");
	}

	/* IsrEvent part */
	spin_lock_irqsave(&DecIsrLock, ulFlags);
	DecIsrEvent.pvHandle = "DECISR_EVENT";
	DecIsrEvent.u4HandleSize = sizeof("DECISR_EVENT")+1;
	DecIsrEvent.u4TimeoutMs = 1;
	spin_unlock_irqrestore(&DecIsrLock, ulFlags);
	eValHWLockRet = eVideoCreateEvent(&DecIsrEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] create dec isr event error\n");
	}

	spin_lock_irqsave(&EncIsrLock, ulFlags);
	EncIsrEvent.pvHandle = "ENCISR_EVENT";
	EncIsrEvent.u4HandleSize = sizeof("ENCISR_EVENT")+1;
	EncIsrEvent.u4TimeoutMs = 1;
	spin_unlock_irqrestore(&EncIsrLock, ulFlags);
	eValHWLockRet = eVideoCreateEvent(&EncIsrEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] create enc isr event error\n");
	}
#if USE_WAKELOCK == 0
	is_entering_suspend = 0;
#endif

	pr_debug("vcodec_driver_init Done\n");

#ifdef CONFIG_MTK_HIBERNATION
	register_swsusp_restore_noirq_func(ID_M_VCODEC,
					vcodec_pm_restore_noirq, NULL);
#endif

	return platform_driver_register(&vcodec_driver);
}

static void __exit vcodec_driver_exit(void)
{
	enum VAL_RESULT_T  eValHWLockRet;

	pr_debug("vcodec_driver_exit\n");
#if USE_WAKELOCK == 1
	mutex_lock(&DriverOpenCountLock);
	wakeup_source_trash(&v_wakeup_src);
	mutex_unlock(&DriverOpenCountLock);
#endif

	mutex_lock(&IsOpenedLock);
	if (bIsOpened == VAL_TRUE) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		bIsOpened = VAL_FALSE;
	}
	mutex_unlock(&IsOpenedLock);

	cdev_del(vcodec_cdev);
	unregister_chrdev_region(vcodec_devno, 1);

	/* [TODO] iounmap the following? */
#if 0
	iounmap((void *)KVA_VENC_IRQ_STATUS_ADDR);
	iounmap((void *)KVA_VENC_IRQ_ACK_ADDR);
#endif

	free_irq(VENC_IRQ_ID, NULL);
	free_irq(VDEC_IRQ_ID, NULL);

	/* MT6589_HWLockEvent part */
	eValHWLockRet = eVideoCloseEvent(&HWLockEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] close dec hwlock event error\n");
	}

	eValHWLockRet = eVideoCloseEvent(&HWLockEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] close enc hwlock event error\n");
	}

	/* MT6589_IsrEvent part */
	eValHWLockRet = eVideoCloseEvent(&DecIsrEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] close dec isr event error\n");
	}

	eValHWLockRet = eVideoCloseEvent(&EncIsrEvent,
					sizeof(struct VAL_EVENT_T));
	if (eValHWLockRet != VAL_RESULT_NO_ERROR) {
		/* Add one line comment for avoid kernel coding style,
		 * WARNING:BRACES:
		 */
		pr_info("[VCODEC][ERROR] close enc isr event error\n");
	}

#ifdef CONFIG_MTK_HIBERNATION
	unregister_swsusp_restore_noirq_func(ID_M_VCODEC);
#endif

	platform_driver_unregister(&vcodec_driver);
}

module_init(vcodec_driver_init);
module_exit(vcodec_driver_exit);
MODULE_AUTHOR("Legis, Lu <legis.lu@mediatek.com>");
MODULE_DESCRIPTION("Vinson Vcodec Driver");
MODULE_LICENSE("GPL");
