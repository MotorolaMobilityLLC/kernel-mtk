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

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/kobject.h>

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <aee.h>

/* Define SMI_INTERNAL_CCF_SUPPORT when CCF needs to be enabled */
#if !defined(CONFIG_MTK_CLKMGR) && !defined(SMI_DUMMY)
#define SMI_INTERNAL_CCF_SUPPORT
#endif

#if defined(SMI_INTERNAL_CCF_SUPPORT)
#include <linux/clk.h>
/* for ccf clk CB */
#if defined(SMI_WHI)
#include "clk-mt6799-pg.h"
#elif defined(SMI_ALA)
#include "clk-mt6759-pg.h"
#elif defined(SMI_BIA)
#include "clk-mt6763-pg.h"
#elif defined(SMI_VIN)
#include "clk-mt6758-pg.h"
#endif
#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */

#include <linux/io.h>

#include <linux/ioctl.h>
#include <linux/fs.h>

#if IS_ENABLED(CONFIG_COMPAT)
#include <linux/uaccess.h>
#include <linux/compat.h>
#endif

#include "mtk_smi.h"

#if defined(SMI_WHI) || defined(SMI_ALA) || defined(SMI_BIA)
#define MMDVFS_HOOK
#endif


#include "smi_reg.h"
#include "smi_common.h"
#include "smi_debug.h"
#include "smi_info_util.h"
#include "smi_configuration.h"
#include "smi_public.h"
#if defined(MMDVFS_HOOK)
#include "mmdvfs_mgr.h"
#include "mmdvfs_config_util.h"
#endif
#undef pr_fmt
#define pr_fmt(fmt) "[SMI]" fmt

#define SMI_LOG_TAG "SMI"

#define LARB_BACKUP_REG_SIZE 128

#define SF_HWC_PIXEL_MAX_NORMAL  (1920 * 1080 * 7)
#define SF_HWC_PIXEL_MAX_VR   (1920 * 1080 * 4 + 1036800)	/* 4.5 FHD size */
#define SF_HWC_PIXEL_MAX_VP   (1920 * 1080 * 7)
#define SF_HWC_PIXEL_MAX_ALWAYS_GPU  (1920 * 1080 * 1)

/* debug level */
static unsigned int smi_debug_level;
static unsigned int mmdvfs_debug_level;
static unsigned int mmdvfs_rt_debug_disable_mask;

/* Record MMDVFS debug level */
unsigned int *g_mmvfs_debug_level = &mmdvfs_debug_level;
unsigned int *g_mmdvfs_rt_debug_disable_mask = &mmdvfs_rt_debug_disable_mask;

#define SMIDBG(level, x...)            \
		do {                        \
			if (smi_debug_level >= (level))    \
				SMIMSG(x);            \
		} while (0)

#define DEFINE_ATTR_RO(_name)\
			static struct kobj_attribute _name##_attr = {\
				.attr	= {\
					.name = #_name,\
					.mode = 0444,\
				},\
				.show	= _name##_show,\
			}

#define DEFINE_ATTR_RW(_name)\
			static struct kobj_attribute _name##_attr = {\
				.attr	= {\
					.name = #_name,\
					.mode = 0644,\
				},\
				.show	= _name##_show,\
				.store	= _name##_store,\
			}

#define __ATTR_OF(_name)	(&_name##_attr.attr)

struct SMI_struct {
	spinlock_t SMI_lock;
	unsigned int pu4ConcurrencyTable[SMI_BWC_SCEN_CNT];	/* one bit represent one module */
};

static struct SMI_struct g_SMIInfo;

/* LARB BASE ADDRESS */
#if defined(SMI_WHI)
unsigned long gLarbBaseAddr[SMI_LARB_NUM] = { 0 };
#endif

static int smi_prepare_count;
static int smi_enable_count;
static atomic_t larbs_clock_count[SMI_LARB_NUM];

static unsigned int smi_first_restore = 1;
char *smi_get_region_name(unsigned int region_indx);

static struct smi_device *smi_dev;

static struct device *smiDeviceUevent;

static struct cdev *pSmiDev;

#define SMI_COMMON_REG_INDX 0
#define SMI_LARB0_REG_INDX 1
#define SMI_LARB1_REG_INDX 2
#define SMI_LARB2_REG_INDX 3
#define SMI_LARB3_REG_INDX 4
#define SMI_LARB4_REG_INDX 5
#define SMI_LARB5_REG_INDX 6
#define SMI_LARB6_REG_INDX 7
#define SMI_LARB7_REG_INDX 8
#define SMI_LARB8_REG_INDX 9
#if defined(SMI_WHI)
#define SMI_MMSYS_REG_INDX 10
static unsigned long mmsys_reg;
#endif

#if defined(SMI_WHI)
#define SMI_REG_REGION_MAX 10
#elif defined(SMI_ALA) || defined(SMI_VIN)
#define SMI_REG_REGION_MAX 9
#elif defined(SMI_BIA)
#define SMI_REG_REGION_MAX 5
#elif defined(SMI_DUMMY)
#define SMI_REG_REGION_MAX 1
#endif

#define SMI_MMIT_PORTING 0

static unsigned long gSMIBaseAddrs[SMI_REG_REGION_MAX];

struct smi_device {
	struct device *dev;
	void __iomem *regs[SMI_REG_REGION_MAX];
#if defined(SMI_WHI)
	void __iomem *mmsys;
#endif
#if defined(SMI_INTERNAL_CCF_SUPPORT)
#if defined(SMI_WHI)
	struct clk *smi_common_fifo0_clk;
	struct clk *smi_common_fifo1_clk;
	struct clk *smi_common_upsz0_clk;
	struct clk *smi_common_upsz1_clk;
	struct clk *smi_common_gals0_clk;
	struct clk *smi_common_gals1_clk;
	struct clk *smi_common_clk;
	struct clk *smi_common_2x_clk;
	struct clk *smi_larb0_clk;
	struct clk *smi_larb1_clk;
	struct clk *smi_larb2_clk;
	struct clk *smi_larb3_clk;
	struct clk *smi_larb4_vdec_clk;
	struct clk *smi_larb4_mm_clk;
	struct clk *smi_larb5_img_clk;
	struct clk *smi_larb5_mm_clk;
	struct clk *smi_larb6_cam_clk;
	struct clk *smi_larb6_mm_clk;
	struct clk *smi_larb7_venc0_clk;
	struct clk *smi_larb7_venc1_clk;
	struct clk *smi_larb7_venc2_clk;
	struct clk *smi_larb7_mm_clk;
	struct clk *smi_larb8_mjc0_clk;
	struct clk *smi_larb8_mjc1_clk;
	struct clk *smi_larb8_mm_clk;
	struct clk *mm_mtcmos;    /* LARB0/1 */
	struct clk *img_mtcmos;   /* LARB2/5 */
	struct clk *cam_mtcmos;   /* LARB3/6 */
	struct clk *vde_mtcmos;   /* LARB4 */
	struct clk *ven_mtcmos;   /* LARB7 */
	struct clk *mjc_mtcmos;   /* LARB8 */
#elif defined(SMI_ALA) || defined(SMI_VIN)
	struct clk *smi_common_gals_comm0_clk;
	struct clk *smi_common_gals_comm1_clk;
	struct clk *smi_common_clk;
	struct clk *smi_larb0_clk;
	struct clk *smi_larb1_clk;
	struct clk *smi_larb2_clk;
	struct clk *gals_ipu2mm_clk;
	struct clk *smi_larb5_clk;
	struct clk *gals_img2mm_clk;
	struct clk *smi_larb3_clk;
	struct clk *smi_larb6_clk;
	struct clk *gals_cam2mm_clk;
	struct clk *smi_larb4_clk;
	struct clk *gals_vdec2mm_clk;
	struct clk *smi_larb7_cke1venc_clk;
	struct clk *smi_larb7_jpgenc_clk;
	struct clk *gals_venc2mm_clk;
	struct clk *mm_mtcmos;    /* LARB0/1 */
	struct clk *img_mtcmos;   /* LARB2/5 */
	struct clk *cam_mtcmos;   /* LARB3/6 */
	struct clk *vde_mtcmos;   /* LARB4 */
	struct clk *ven_mtcmos;   /* LARB7 */
#elif defined(SMI_BIA)
	struct clk *smi_common_gals_comm0_clk;
	struct clk *smi_common_gals_comm1_clk;
	struct clk *smi_common_clk;
	struct clk *smi_larb0_clk;
	struct clk *smi_larb1_clk;
	struct clk *smi_larb2_clk;
	struct clk *gals_cam2mm_clk;
	struct clk *smi_larb3_clk_larb;
	struct clk *smi_larb3_clk_venc;
	struct clk *mm_mtcmos;    /* LARB0 */
	struct clk *img_mtcmos;   /* LARB1 */
	struct clk *cam_mtcmos;   /* LARB2 */
	struct clk *ven_mtcmos;   /* LARB3 */
#endif
#endif
};


static unsigned int wifi_disp_transaction;

/* larb backuprestore */
#if defined(SMI_INTERNAL_CCF_SUPPORT)
static bool fglarbcallback;
#endif
/* tuning mode, 1 for register ioctl */
static unsigned int enable_ioctl;
static unsigned int disable_freq_hopping = 1;
static unsigned int disable_freq_mux = 1;
static unsigned int force_max_mmsys_clk;
static unsigned int force_camera_hpm;
static unsigned int bus_optimization;
static unsigned int enable_bw_optimization;
static unsigned int smi_profile = SMI_BWC_SCEN_NORMAL;
static unsigned int disable_mmdvfs;
static unsigned int clk_mux_mask;
static unsigned int *pLarbRegBackUp[SMI_LARB_NUM];
/* force_always_on_mm_clks_mask function is debug only */
static unsigned int force_always_on_mm_clks_mask = 1;

struct MTK_SMI_BWC_MM_INFO g_smi_bwc_mm_info = {
	0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 0, 0, 0,
	SF_HWC_PIXEL_MAX_NORMAL
};

enum smi_clk_operation {
	SMI_PREPARE_CLK,
	SMI_ENABLE_CLK,
	SMI_DISABLE_CLK,
	SMI_UNPREPARE_CLK
};

static void smi_common_clk_operation(enum smi_clk_operation op);
static unsigned long get_register_base(int i);
static void smi_driver_setting(void);
static char *smi_get_scenario_name(enum MTK_SMI_BWC_SCEN scen);
static void smi_bus_optimization_clock_control(int optimization_larbs, enum smi_clk_operation oper);
static void smi_apply_common_basic_setting(void);
static void smi_apply_larb_basic_setting(int larb);
static void smi_apply_basic_setting(void);
static void smi_apply_larb_mmu_setting(int larb);
static void smi_apply_mmu_setting(void);


#if defined(SMI_INTERNAL_CCF_SUPPORT)
static struct clk *get_smi_clk(char *smi_clk_name);
#endif

#if IS_ENABLED(CONFIG_COMPAT)
static long MTK_SMI_COMPAT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
#define MTK_SMI_COMPAT_ioctl  NULL
#endif


/* Use this function to get base address of Larb resgister */
/* to support error checking */
unsigned long get_larb_base_addr(int larb_id)
{
	if (larb_id >= SMI_LARB_NUM || larb_id < 0)
		return SMI_ERROR_ADDR;

#if defined(SMI_WHI)
	return gLarbBaseAddr[larb_id];
#else
	/* larb index starts from 1 */
	return gSMIBaseAddrs[larb_id + SMI_LARB0_REG_INDX];
#endif
}

unsigned long get_common_base_addr(void)
{
	return gSMIBaseAddrs[SMI_COMMON_REG_INDX];
}

#if defined(SMI_INTERNAL_CCF_SUPPORT)
struct clk *get_smi_clk(char *smi_clk_name)
{
	struct clk *smi_clk_ptr = NULL;

	smi_clk_ptr = devm_clk_get(smi_dev->dev, smi_clk_name);
	if (IS_ERR(smi_clk_ptr)) {
		SMIMSG("cannot get %s\n", smi_clk_name);
		smi_clk_ptr = NULL;
	}
	return smi_clk_ptr;
}

unsigned int get_larb_clock_count(const int larb_id)
{
	if (larb_id < SMI_LARB_NUM)
		return (unsigned int)atomic_read(&(larbs_clock_count[larb_id]));
	return 0;
}

#if !defined(CONFIG_FPGA_EARLY_PORTING)
static void smi_prepare_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		int ret = 0;

		ret = clk_prepare(smi_clk);
		if (ret) {
			SMIMSG("clk_prepare return error %d, %s\n", ret, name);
		} else {
			smi_prepare_count++;
			SMIDBG(3, "clk:%s prepare done.\n", name);
			SMIDBG(3, "smi_prepare_count=%d\n", smi_prepare_count);
		}
	} else {
		SMIMSG("clk_prepare error, smi_clk can't be NULL, %s\n", name);
	}
}

static void smi_enable_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		int ret = 0;

		ret = clk_enable(smi_clk);
		if (ret) {
			SMIMSG("clk_enable return error %d, %s\n", ret, name);
		} else {
			smi_enable_count++;
			SMIDBG(3, "clk:%s enable done.\n", name);
			SMIDBG(3, "smi_enable_count=%d\n", smi_enable_count);
		}
	} else {
		SMIMSG("clk_enable error, smi_clk can't be NULL, %s\n", name);
	}
}

static void smi_unprepare_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		clk_unprepare(smi_clk);
		smi_prepare_count--;
		SMIDBG(3, "clk:%s unprepare done.\n", name);
		SMIDBG(3, "smi_prepare_count=%d\n", smi_prepare_count);
	} else {
		SMIMSG("smi_unprepare error, smi_clk can't be NULL, %s\n", name);
	}
}

static void smi_disable_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		clk_disable(smi_clk);
		smi_enable_count--;
		SMIDBG(3, "clk:%s disable done.\n", name);
		SMIDBG(3, "smi_enable_count=%d\n", smi_enable_count);
	} else {
		SMIMSG("smi_disable error, smi_clk can't be NULL, %s\n", name);
	}
}

#endif /* !defined(CONFIG_FPGA_EARLY_PORTING) */
/* end MTCMOS*/
#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */

static void smi_common_clk_operation(enum smi_clk_operation op)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	switch (op) {
	case SMI_PREPARE_CLK:
#if defined(SMI_WHI)
		smi_prepare_clk(smi_dev->smi_common_gals0_clk, "smi_common_gals0_clk");
		smi_prepare_clk(smi_dev->smi_common_gals1_clk, "smi_common_gals1_clk");
		smi_prepare_clk(smi_dev->smi_common_fifo0_clk, "smi_common_fifo0_clk");
		smi_prepare_clk(smi_dev->smi_common_fifo1_clk, "smi_common_fifo1_clk");
		smi_prepare_clk(smi_dev->smi_common_upsz0_clk, "smi_common_upsz0_clk");
		smi_prepare_clk(smi_dev->smi_common_upsz1_clk, "smi_common_upsz1_clk");
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_common_2x_clk, "smi_common_2x_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_prepare_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
		smi_prepare_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
#elif defined(SMI_BIA)
		smi_prepare_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
		smi_prepare_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
#endif
		break;
	case SMI_ENABLE_CLK:
#if defined(SMI_WHI)
		smi_enable_clk(smi_dev->smi_common_gals0_clk, "smi_common_gals0_clk");
		smi_enable_clk(smi_dev->smi_common_gals1_clk, "smi_common_gals1_clk");
		smi_enable_clk(smi_dev->smi_common_fifo0_clk, "smi_common_fifo0_clk");
		smi_enable_clk(smi_dev->smi_common_fifo1_clk, "smi_common_fifo1_clk");
		smi_enable_clk(smi_dev->smi_common_upsz0_clk, "smi_common_upsz0_clk");
		smi_enable_clk(smi_dev->smi_common_upsz1_clk, "smi_common_upsz1_clk");
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_common_2x_clk, "smi_common_2x_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_enable_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
		smi_enable_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
#elif defined(SMI_BIA)
		smi_enable_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
		smi_enable_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
#endif
		break;
	case SMI_DISABLE_CLK:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_common_2x_clk, "smi_common_2x_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_disable_clk(smi_dev->smi_common_upsz1_clk, "smi_common_upsz1_clk");
		smi_disable_clk(smi_dev->smi_common_upsz0_clk, "smi_common_upsz0_clk");
		smi_disable_clk(smi_dev->smi_common_fifo1_clk, "smi_common_fifo1_clk");
		smi_disable_clk(smi_dev->smi_common_fifo0_clk, "smi_common_fifo0_clk");
		smi_disable_clk(smi_dev->smi_common_gals1_clk, "smi_common_gals1_clk");
		smi_disable_clk(smi_dev->smi_common_gals0_clk, "smi_common_gals0_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_disable_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_disable_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
#elif defined(SMI_BIA)
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_disable_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_disable_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
#endif
		break;
	case SMI_UNPREPARE_CLK:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_common_2x_clk, "smi_common_2x_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_unprepare_clk(smi_dev->smi_common_upsz1_clk, "smi_common_upsz1_clk");
		smi_unprepare_clk(smi_dev->smi_common_upsz0_clk, "smi_common_upsz0_clk");
		smi_unprepare_clk(smi_dev->smi_common_fifo1_clk, "smi_common_fifo1_clk");
		smi_unprepare_clk(smi_dev->smi_common_fifo0_clk, "smi_common_fifo0_clk");
		smi_unprepare_clk(smi_dev->smi_common_gals1_clk, "smi_common_gals1_clk");
		smi_unprepare_clk(smi_dev->smi_common_gals0_clk, "smi_common_gals0_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_unprepare_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_unprepare_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
#elif defined(SMI_BIA)
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_unprepare_clk(smi_dev->smi_common_gals_comm1_clk, "smi_common_gals_comm1_clk");
		smi_unprepare_clk(smi_dev->smi_common_gals_comm0_clk, "smi_common_gals_comm0_clk");
#endif
		break;
	default:
		break;
	}
#endif
}

static int larb_clock_enable(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(SMI_DUMMY)
	char name[30];

	sprintf(name, "larb%d", larb_id);

	switch (larb_id) {
	case 0:
#if defined(SMI_WHI)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
#endif
		break;
	case 1:
#if defined(SMI_WHI)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
#endif
		break;
	case 2:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_enable_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_enable_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
#endif
		break;
	case 3:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
		smi_enable_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb3_clk_larb, "smi_larb3_clk_larb");
		smi_enable_clk(smi_dev->smi_larb3_clk_venc, "smi_larb3_clk_venc");
#endif
		break;
	case 4:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb4_vdec_clk, "smi_larb4_vdec_clk");
		smi_enable_clk(smi_dev->smi_larb4_mm_clk, "smi_larb4_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
		smi_enable_clk(smi_dev->gals_vdec2mm_clk, "gals_vdec2mm_clk");
#endif
		break;
	case 5:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb5_img_clk, "smi_larb5_img_clk");
		smi_enable_clk(smi_dev->smi_larb5_mm_clk, "smi_larb5_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_enable_clk(smi_dev->gals_img2mm_clk, "gals_img2mm_clk");
#endif
		break;
	case 6:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb6_cam_clk, "smi_larb6_cam_clk");
		smi_enable_clk(smi_dev->smi_larb6_mm_clk, "smi_larb6_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		smi_enable_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
#endif
		break;
	case 7:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb7_venc0_clk, "smi_larb7_venc0_clk");
		smi_enable_clk(smi_dev->smi_larb7_venc1_clk, "smi_larb7_venc1_clk");
		smi_enable_clk(smi_dev->smi_larb7_venc2_clk, "smi_larb7_venc2_clk");
		smi_enable_clk(smi_dev->smi_larb7_mm_clk, "smi_larb7_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb7_cke1venc_clk, "smi_larb7_cke1venc_clk");
		smi_enable_clk(smi_dev->smi_larb7_jpgenc_clk, "smi_larb7_jpgenc_clk");
		smi_enable_clk(smi_dev->gals_venc2mm_clk, "gals_venc2mm_clk");
#endif
		break;
	case 8:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_enable_clk(smi_dev->mjc_mtcmos, "mjc_mtcmos");
		}
		smi_common_clk_operation(SMI_ENABLE_CLK);
		smi_enable_clk(smi_dev->smi_larb8_mjc0_clk, "smi_larb8_mjc0_clk");
		smi_enable_clk(smi_dev->smi_larb8_mjc1_clk, "smi_larb8_mjc1_clk");
		smi_enable_clk(smi_dev->smi_larb8_mm_clk, "smi_larb8_mm_clk");
#endif
		break;
	default:
		break;
	}
#else
	SMIMSG("larb_clock_enable is not available\n");
#endif
	if (larb_id < SMI_LARB_NUM)
		atomic_inc(&(larbs_clock_count[larb_id]));
	return 0;
}

static int larb_clock_prepare(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && defined(SMI_INTERNAL_CCF_SUPPORT)
	char name[30];

	sprintf(name, "larb%d", larb_id);

	switch (larb_id) {
	case 0:
#if defined(SMI_WHI)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
#endif
		break;
	case 1:
#if defined(SMI_WHI)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
#endif
		break;
	case 2:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_prepare_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_prepare_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
#endif
		break;
	case 3:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
		smi_prepare_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
#elif defined(SMI_BIA)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb3_clk_larb, "smi_larb3_clk_larb");
		smi_prepare_clk(smi_dev->smi_larb3_clk_venc, "smi_larb3_clk_venc");
#endif
		break;
	case 4:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb4_vdec_clk, "smi_larb4_vdec_clk");
		smi_prepare_clk(smi_dev->smi_larb4_mm_clk, "smi_larb4_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
		smi_prepare_clk(smi_dev->gals_vdec2mm_clk, "gals_vdec2mm_clk");
#endif
		break;
	case 5:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb5_img_clk, "smi_larb5_img_clk");
		smi_prepare_clk(smi_dev->smi_larb5_mm_clk, "smi_larb5_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_prepare_clk(smi_dev->gals_img2mm_clk, "gals_img2mm_clk");
#endif
		break;
	case 6:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb6_cam_clk, "smi_larb6_cam_clk");
		smi_prepare_clk(smi_dev->smi_larb6_mm_clk, "smi_larb6_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		smi_prepare_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
#endif
		break;
	case 7:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb7_venc0_clk, "smi_larb7_venc0_clk");
		smi_prepare_clk(smi_dev->smi_larb7_venc1_clk, "smi_larb7_venc1_clk");
		smi_prepare_clk(smi_dev->smi_larb7_venc2_clk, "smi_larb7_venc2_clk");
		smi_prepare_clk(smi_dev->smi_larb7_mm_clk, "smi_larb7_mm_clk");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb7_cke1venc_clk, "smi_larb7_cke1venc_clk");
		smi_prepare_clk(smi_dev->smi_larb7_jpgenc_clk, "smi_larb7_jpgenc_clk");
		smi_prepare_clk(smi_dev->gals_venc2mm_clk, "gals_venc2mm_clk");
#endif
		break;
	case 8:
#if defined(SMI_WHI)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
			smi_prepare_clk(smi_dev->mjc_mtcmos, "mjc_mtcmos");
		}
		smi_common_clk_operation(SMI_PREPARE_CLK);
		smi_prepare_clk(smi_dev->smi_larb8_mjc0_clk, "smi_larb8_mjc0_clk");
		smi_prepare_clk(smi_dev->smi_larb8_mjc1_clk, "smi_larb8_mjc1_clk");
		smi_prepare_clk(smi_dev->smi_larb8_mm_clk, "smi_larb8_mm_clk");
		break;
#endif
	default:
		break;
	}
#else
	SMIMSG("larb_clock_prepare is not available\n");
#endif
	return 0;
}

static int larb_clock_disable(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(SMI_DUMMY)
	char name[30];

	sprintf(name, "larb%d", larb_id);

	switch (larb_id) {
	case 0:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_BIA)
		smi_disable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#endif
		break;
	case 1:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_BIA)
		smi_disable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 2:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
		smi_disable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_BIA)
		smi_disable_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
		smi_disable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 3:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
		smi_disable_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_BIA)
		smi_disable_clk(smi_dev->smi_larb3_clk_venc, "smi_larb3_clk_venc");
		smi_disable_clk(smi_dev->smi_larb3_clk_larb, "smi_larb3_clk_larb");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 4:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb4_mm_clk, "smi_larb4_mm_clk");
		smi_disable_clk(smi_dev->smi_larb4_vdec_clk, "smi_larb4_vdec_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->gals_vdec2mm_clk, "gals_vdec2mm_clk");
		smi_disable_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 5:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb5_mm_clk, "smi_larb5_mm_clk");
		smi_disable_clk(smi_dev->smi_larb5_img_clk, "smi_larb5_img_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->gals_img2mm_clk, "gals_img2mm_clk");
		smi_disable_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 6:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb6_mm_clk, "smi_larb6_mm_clk");
		smi_disable_clk(smi_dev->smi_larb6_cam_clk, "smi_larb6_cam_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
		smi_disable_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 7:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb7_mm_clk, "smi_larb7_mm_clk");
		smi_disable_clk(smi_dev->smi_larb7_venc2_clk, "smi_larb7_venc2_clk");
		smi_disable_clk(smi_dev->smi_larb7_venc1_clk, "smi_larb7_venc1_clk");
		smi_disable_clk(smi_dev->smi_larb7_venc0_clk, "smi_larb7_venc0_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_disable_clk(smi_dev->gals_venc2mm_clk, "gals_venc2mm_clk");
		smi_disable_clk(smi_dev->smi_larb7_jpgenc_clk, "smi_larb7_jpgenc_clk");
		smi_disable_clk(smi_dev->smi_larb7_cke1venc_clk, "smi_larb7_cke1venc_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 8:
#if defined(SMI_WHI)
		smi_disable_clk(smi_dev->smi_larb8_mm_clk, "smi_larb8_mm_clk");
		smi_disable_clk(smi_dev->smi_larb8_mjc1_clk, "smi_larb8_mjc1_clk");
		smi_disable_clk(smi_dev->smi_larb8_mjc0_clk, "smi_larb8_mjc0_clk");
		smi_common_clk_operation(SMI_DISABLE_CLK);
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->mjc_mtcmos, "mjc_mtcmos");
			smi_disable_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	default:
		break;
	}
#else
	SMIMSG("larb_clock_disable is not available\n");
#endif
	if (larb_id < SMI_LARB_NUM)
		atomic_dec(&(larbs_clock_count[larb_id]));
	return 0;
}

static int larb_clock_unprepare(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && defined(SMI_INTERNAL_CCF_SUPPORT)
	char name[30];

	sprintf(name, "larb%d", larb_id);
	switch (larb_id) {
	case 0:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_BIA)
		smi_unprepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#endif
		break;
	case 1:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
#elif defined(SMI_BIA)
		smi_unprepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 2:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_BIA)
		smi_unprepare_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 3:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->gals_ipu2mm_clk, "gals_ipu2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb3_clk, "smi_larb3_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_BIA)
		smi_unprepare_clk(smi_dev->smi_larb3_clk_venc, "smi_larb3_clk_venc");
		smi_unprepare_clk(smi_dev->smi_larb3_clk_larb, "smi_larb3_clk_larb");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 4:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb4_mm_clk, "smi_larb4_mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb4_vdec_clk, "smi_larb4_vdec_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->gals_vdec2mm_clk, "gals_vdec2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->vde_mtcmos, "vde_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 5:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb5_mm_clk, "smi_larb5_mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb5_img_clk, "smi_larb5_img_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->gals_img2mm_clk, "gals_img2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->img_mtcmos, "img_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 6:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb6_mm_clk, "smi_larb6_mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb6_cam_clk, "smi_larb6_cam_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->gals_cam2mm_clk, "gals_cam2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->cam_mtcmos, "cam_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 7:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb7_mm_clk, "smi_larb7_mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb7_venc2_clk, "smi_larb7_venc2_clk");
		smi_unprepare_clk(smi_dev->smi_larb7_venc1_clk, "smi_larb7_venc1_clk");
		smi_unprepare_clk(smi_dev->smi_larb7_venc0_clk, "smi_larb7_venc0_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_unprepare_clk(smi_dev->gals_venc2mm_clk, "gals_venc2mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb7_jpgenc_clk, "smi_larb7_jpgenc_clk");
		smi_unprepare_clk(smi_dev->smi_larb7_cke1venc_clk, "smi_larb7_cke1venc_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->ven_mtcmos, "ven_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	case 8:
#if defined(SMI_WHI)
		smi_unprepare_clk(smi_dev->smi_larb8_mm_clk, "smi_larb8_mm_clk");
		smi_unprepare_clk(smi_dev->smi_larb8_mjc1_clk, "smi_larb8_mjc1_clk");
		smi_unprepare_clk(smi_dev->smi_larb8_mjc0_clk, "smi_larb8_mjc0_clk");
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->mjc_mtcmos, "mjc_mtcmos");
			smi_unprepare_clk(smi_dev->mm_mtcmos, "mm_mtcmos");
		}
#endif
		break;
	default:
		break;
	}
#else
	SMIMSG("larb_clock_unprepare is not available\n");
#endif
	return 0;
}

/*
 * prepare and enable CG/MTCMOS of specific LARB and COMMON
 * larb_id: used for select specific larb
 * user_name: caller's module name, used for debug
 */
int smi_bus_enable(enum SMI_MASTER_ID master_id, char *user_name)
{
	smi_clk_prepare(master_id, user_name, 1);
	smi_clk_enable(master_id, user_name, 1);
	return 0;
}

/*
 * disable and unprepare CG/MTCMOS of specific LARB and COMMON
 * larb_id: used for select specific larb
 * user_name: caller's module name, used for debug
 */
int smi_bus_disable(enum SMI_MASTER_ID master_id, char *user_name)
{
	smi_clk_disable(master_id, user_name, 1);
	smi_clk_unprepare(master_id, user_name, 1);
	return 0;
}

/*
 * prepare CG/MTCMOS of specific LARB and COMMON
 * larb_id: used for select specific larb
 * user_name: caller's module name, used for debug
 * enable_mtcmos: wish to manipulate mtcmos = 1, else = 0
 */
int smi_clk_prepare(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIDBG(1, "%s is calling smi_clk_prepare\n", user_name);
	if (master_id == SMI_COMMON)
		smi_common_clk_operation(SMI_PREPARE_CLK);
	else
		larb_clock_prepare(master_id, enable_mtcmos);
	return 0;
}

/*
 * enable CG/MTCMOS of specific LARB and COMMON
 * larb_id: used for select specific larb
 * user_name: caller's module name, used for debug
 * enable_mtcmos: wish to manipulate mtcmos = 1, else = 0
 */
int smi_clk_enable(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIDBG(1, "%s is calling smi_clk_enable\n", user_name);
	if (master_id == SMI_COMMON)
		smi_common_clk_operation(SMI_ENABLE_CLK);
	else
		larb_clock_enable(master_id, enable_mtcmos);
	return 0;
}

/*
 * unprepare CG/MTCMOS of specific LARB and COMMON
 * larb_id: used for select specific larb
 * user_name: caller's module name, used for debug
 * enable_mtcmos: wish to manipulate mtcmos = 1, else = 0
 */
int smi_clk_unprepare(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIDBG(1, "%s is calling smi_clk_unprepare\n", user_name);
	if (master_id == SMI_COMMON)
		smi_common_clk_operation(SMI_UNPREPARE_CLK);
	else
		larb_clock_unprepare(master_id, enable_mtcmos);
	return 0;
}

/*
 * disable CG/MTCMOS of specific LARB and COMMON
 * master_id: used for select specific larb
 * user_name: caller's module name, used for debug
 * enable_mtcmos: wish to manipulate mtcmos = 1, else = 0
 */
int smi_clk_disable(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIDBG(1, "%s is calling smi_clk_disable\n", user_name);
	if (master_id == SMI_COMMON)
		smi_common_clk_operation(SMI_DISABLE_CLK);
	else
		larb_clock_disable(master_id, enable_mtcmos);
	return 0;
}

int larb_reg_restore(int larb)
{
	int larb_by_bit = 1 << larb;

	SMIDBG(1, "larb_reg_restore is called, restore larb%d\n", larb);

	smi_bus_regs_setting(larb_by_bit, smi_profile,
			smi_profile_config[smi_profile].setting);

	if (larb == 0) {
		/* common will disable when larb0 disable */
		smi_apply_common_basic_setting();
	}

	smi_apply_larb_basic_setting(larb_by_bit);
	smi_apply_larb_mmu_setting(larb_by_bit);

	return 0;
}

#if defined(SMI_INTERNAL_CCF_SUPPORT)
void on_larb_power_on_with_ccf(int larb_idx)
{
	/* MTCMOS has already enable, only enable clk here to set register value */
	if (larb_idx < 0 || larb_idx >= SMI_LARB_NUM) {
		SMIMSG("incorrect larb:%d\n", larb_idx);
		return;
	}

	larb_clock_prepare(larb_idx, 0);
	larb_clock_enable(larb_idx, 0);
	larb_reg_restore(larb_idx);
	larb_clock_disable(larb_idx, 0);
	larb_clock_unprepare(larb_idx, 0);
}

#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */




/* Fake mode check, e.g. WFD */
static int fake_mode_handling(struct MTK_SMI_BWC_CONFIG *p_conf, unsigned int *pu4LocalCnt)
{
	if (p_conf->scenario == SMI_BWC_SCEN_WFD) {
		if (p_conf->b_on_off) {
			wifi_disp_transaction = 1;
			SMIMSG("Enable WFD in profile: %d\n", smi_profile);
		} else {
			wifi_disp_transaction = 0;
			SMIMSG("Disable WFD in profile: %d\n", smi_profile);
		}
		return 1;
	} else {
		return 0;
	}
}

static int ovl_limit_uevent(int bwc_scenario, int ovl_pixel_limit)
{
	int err = 0;
#if 0
	char *envp[3];
	char scenario_buf[32] = "";
	char ovl_limit_buf[32] = "";

	snprintf(scenario_buf, 31, "SCEN=%d", bwc_scenario);
	snprintf(ovl_limit_buf, 31, "HWOVL=%d", ovl_pixel_limit);

	envp[0] = scenario_buf;
	envp[1] = ovl_limit_buf;
	envp[2] = NULL;

	if (pSmiDev) {
		err = kobject_uevent_env(&(smiDeviceUevent->kobj), KOBJ_CHANGE, envp);
		SMIMSG("Notify OVL limitaion=%d, SCEN=%d", ovl_pixel_limit, bwc_scenario);
	}
#endif
	if (err < 0)
		SMIMSG(KERN_INFO "[%s] kobject_uevent_env error = %d\n", __func__, err);

	return err;
}

#if defined(SMI_INTERNAL_CCF_SUPPORT)
static unsigned int smiclk_subsys_2_larb(enum subsys_id sys)
{
	unsigned int i4larbid = 0;
#if defined(SMI_WHI)
	switch (sys) {
	case SYS_MM0:
		i4larbid = 0x3;    /* larb0/1 */
		break;
	case SYS_VDE:
		i4larbid = 0x10;   /* larb4 */
		break;
	case SYS_ISP:
		i4larbid = 0x24;   /* larb2/5 */
		break;
	case SYS_VEN:
		i4larbid = 0x80;   /* larb7 */
	break;
	case SYS_MJC:
		i4larbid = 0x100;  /* larb8 */
		break;
	case SYS_CAM:
		i4larbid = 0x48;   /* larb3/6 */
		break;
	default:
		break;
	}
#elif defined(SMI_ALA) || defined(SMI_VIN)
	switch (sys) {
	case SYS_MM0:
		i4larbid = 0x3;    /* larb0/1 */
		break;
	case SYS_VDE:
		i4larbid = 0x10;   /* larb4 */
		break;
	case SYS_ISP:
		i4larbid = 0x24;   /* larb2/5 */
		break;
	case SYS_VEN:
		i4larbid = 0x80;   /* larb7 */
	break;
	case SYS_CAM:
		i4larbid = 0x48;   /* larb3/6 */
		break;
	default:
		break;
	}
#elif defined(SMI_BIA)
	switch (sys) {
	case SYS_DIS:
		i4larbid = 0x1;    /* larb0 */
		break;
	case SYS_ISP:
		i4larbid = 0x2;   /* larb1 */
		break;
	case SYS_VEN:
		i4larbid = 0x8;   /* larb3 */
	break;
	case SYS_CAM:
		i4larbid = 0x4;   /* larb2 */
		break;
	default:
		break;
	}
#endif
	return i4larbid;
}

static void smiclk_subsys_after_on(enum subsys_id sys)
{
	unsigned int i4larbid = smiclk_subsys_2_larb(sys);
	int i = 0;

	if (!fglarbcallback) {
		SMIDBG(1, "don't need restore incb\n");
		return;
	}

	do {
		if ((i4larbid & 1) && (1 << i & bus_optimization)) {
			if (i < SMI_LARB_NUM) {
				SMIDBG(1, "ready to call restore with larb%d.\n", i);
				on_larb_power_on_with_ccf(i);
				}
		}
		i4larbid = i4larbid >> 1;
		i++;
	} while (i4larbid != 0);
}

struct pg_callbacks smi_clk_subsys_handle = {
	.after_on = smiclk_subsys_after_on
};

#endif /* SMI_INTERNAL_CCF_SUPPORT */

/* prepare larb clk because prepare cannot in spinlock */
static void smi_bus_optimization_prepare(int optimization_larbs)
{
	int i = 0;

	for (i = 0; i < SMI_LARB_NUM; i++) {
		int larb_mask = 1 << i;

		if (optimization_larbs & larb_mask) {
			SMIDBG(1, "prepare clock%d\n", i);
			smi_clk_prepare(i, "SMI", 1);
		}
	}
}

/* unprepare larb clk because prepare cannot in spinlock */
static void smi_bus_optimization_unprepare(int optimization_larbs)
{
	int i = 0;

	for (i = 0; i < SMI_LARB_NUM; i++) {
		int larb_mask = 1 << i;

		if (optimization_larbs & larb_mask) {
			SMIDBG(1, "unprepare clock%d\n", i);
			smi_clk_unprepare(i, "SMI", 1);
		}
	}
}
static void smi_bus_optimization_enable(int optimization_larbs)
{
	int i = 0;

	for (i = 0; i < SMI_LARB_NUM; i++) {
		int larb_mask = 1 << i;

		if (optimization_larbs & larb_mask) {
			SMIDBG(1, "enable clock%d\n", i);
			smi_clk_enable(i, "SMI", 1);
		}
	}
}
static void smi_bus_optimization_disable(int optimization_larbs)
{
	int i = 0;

	for (i = 0; i < SMI_LARB_NUM; i++) {
		int larb_mask = 1 << i;

		if (optimization_larbs & larb_mask) {
			SMIDBG(1, "disable clock%d\n", i);
			smi_clk_disable(i, "SMI", 1);
		}
	}
}
/* used to control clock/MTCMOS */
static void smi_bus_optimization_clock_control(int optimization_larbs, enum smi_clk_operation oper)
{
	switch (oper) {
	case SMI_PREPARE_CLK:
		smi_bus_optimization_prepare(optimization_larbs);
		break;
	case SMI_ENABLE_CLK:
		smi_bus_optimization_enable(optimization_larbs);
		break;
	case SMI_DISABLE_CLK:
		smi_bus_optimization_disable(optimization_larbs);
		break;
	case SMI_UNPREPARE_CLK:
		smi_bus_optimization_unprepare(optimization_larbs);
		break;
	default:
		break;
	}
}

static void smi_bus_optimization(int optimization_larbs, int smi_profile)
{
	if (enable_bw_optimization) {
		SMIDBG(1, "dump register before setting\n");
		if (smi_debug_level)
			smi_dumpDebugMsg();

		smi_bus_regs_setting(optimization_larbs, smi_profile,
			smi_profile_config[smi_profile].setting);

		SMIDBG(1, "dump register after setting\n");
		if (smi_debug_level)
			smi_dumpDebugMsg();

	}
}
static char *smi_get_scenario_name(enum MTK_SMI_BWC_SCEN scen)
{
	switch (scen) {
	case SMI_BWC_SCEN_NORMAL:
		return "SMI_BWC_SCEN_NORMAL";
	case SMI_BWC_SCEN_VR:
		return "SMI_BWC_SCEN_VR";
	case SMI_BWC_SCEN_SWDEC_VP:
		return "SMI_BWC_SCEN_SWDEC_VP";
	case SMI_BWC_SCEN_VP:
		return "SMI_BWC_SCEN_VP";
	case SMI_BWC_SCEN_VP_HIGH_FPS:
		return "SMI_BWC_SCEN_VP_HIGH_FPS";
	case SMI_BWC_SCEN_VP_HIGH_RESOLUTION:
		return "SMI_BWC_SCEN_VP_HIGH_RESOLUTION";
	case SMI_BWC_SCEN_VR_SLOW:
		return "SMI_BWC_SCEN_VR_SLOW";
	case SMI_BWC_SCEN_MM_GPU:
		return "SMI_BWC_SCEN_MM_GPU";
	case SMI_BWC_SCEN_WFD:
		return "SMI_BWC_SCEN_WFD";
	case SMI_BWC_SCEN_VENC:
		return "SMI_BWC_SCEN_VENC";
	case SMI_BWC_SCEN_ICFP:
		return "SMI_BWC_SCEN_ICFP";
	case SMI_BWC_SCEN_UI_IDLE:
		return "SMI_BWC_SCEN_UI_IDLE";
	case SMI_BWC_SCEN_VSS:
		return "SMI_BWC_SCEN_VSS";
	case SMI_BWC_SCEN_FORCE_MMDVFS:
		return "SMI_BWC_SCEN_FORCE_MMDVFS";
	case SMI_BWC_SCEN_HDMI:
		return "SMI_BWC_SCEN_HDMI";
	case SMI_BWC_SCEN_HDMI4K:
		return "SMI_BWC_SCEN_HDMI4K";
	case SMI_BWC_SCEN_VPMJC:
		return "SMI_BWC_SCEN_VPMJC";
	case SMI_BWC_SCEN_N3D:
		return "SMI_BWC_SCEN_N3D";
	case SMI_BWC_SCEN_CAM_PV:
		return "SMI_BWC_SCEN_CAM_PV";
	case SMI_BWC_SCEN_CAM_CP:
		return "SMI_BWC_SCEN_CAM_CP";
	default:
		return "unknown scenario";
	}
	return "";
}
static int smi_bwc_config(struct MTK_SMI_BWC_CONFIG *p_conf, unsigned int *pu4LocalCnt)
{
	int i;
	int result = 0;
	unsigned int u4Concurrency = 0;
	int bus_optimization_sync = bus_optimization;
	enum MTK_SMI_BWC_SCEN eFinalScen;
	static enum MTK_SMI_BWC_SCEN ePreviousFinalScen = SMI_BWC_SCEN_CNT;

	if ((p_conf->scenario >= SMI_BWC_SCEN_CNT) || (p_conf->scenario < 0)) {
		SMIERR("Incorrect SMI BWC config : 0x%x, how could this be...\n", p_conf->scenario);
		return -1;
	}

	SMIDBG(3, "current request is turn %s %d\n", p_conf->b_on_off ? "on" : "off", p_conf->scenario);
#ifdef MMDVFS_HOOK
	if (!disable_mmdvfs) {
		if (p_conf->b_on_off) {
			/* set mmdvfs step according to certain scenarios */
			mmdvfs_notify_scenario_enter(p_conf->scenario);
		} else {
			/* set mmdvfs step to default after the scenario exits */
			mmdvfs_notify_scenario_exit(p_conf->scenario);
		}
	}
#endif

	spin_lock(&g_SMIInfo.SMI_lock);

	result = fake_mode_handling(p_conf, pu4LocalCnt);

	if (enable_bw_optimization)
		bus_optimization_sync = bus_optimization;
	else
		bus_optimization_sync = 0;


	spin_unlock(&g_SMIInfo.SMI_lock);

	/* Fake mode is not a real SMI profile, so we need to return here */
	if (result == 1)
		return 0;

	smi_bus_optimization_clock_control(bus_optimization_sync, SMI_PREPARE_CLK);

	spin_lock(&g_SMIInfo.SMI_lock);

	if (p_conf->b_on_off) {
		/* turn on certain scenario */
		g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] += 1;

		if (pu4LocalCnt)
			pu4LocalCnt[p_conf->scenario] += 1;

	} else {
		/* turn off certain scenario */
		if (g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] == 0) {
			SMIMSG("Too many turning off for global SMI profile:%d,%d\n",
			       p_conf->scenario, g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario]);
		} else {
			g_SMIInfo.pu4ConcurrencyTable[p_conf->scenario] -= 1;
		}

		if (pu4LocalCnt) {
			if (pu4LocalCnt[p_conf->scenario] == 0) {
				SMIMSG
				    ("Process : %s did too many turning off for local SMI profile:%d,%d\n",
				     current->comm, p_conf->scenario,
				     pu4LocalCnt[p_conf->scenario]);
			} else {
				pu4LocalCnt[p_conf->scenario] -= 1;
			}
		}
	}

	for (i = 0; i < SMI_BWC_SCEN_CNT; i++) {
		if (g_SMIInfo.pu4ConcurrencyTable[i])
			u4Concurrency |= (1 << i);
	}

	SMIDBG(3, "after update, u4Concurrency=0x%x\n", u4Concurrency);
#ifdef MMDVFS_HOOK
	/* notify mmdvfs concurrency */
	if (!disable_mmdvfs)
		mmdvfs_notify_scenario_concurrency(u4Concurrency);
#endif

	if ((1 << SMI_BWC_SCEN_MM_GPU) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_MM_GPU;
	else if ((1 << SMI_BWC_SCEN_ICFP) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_ICFP;
	else if ((1 << SMI_BWC_SCEN_VSS) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VSS;
	else if ((1 << SMI_BWC_SCEN_VR_SLOW) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VR_SLOW;
	else if ((1 << SMI_BWC_SCEN_VR) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VR;
	else if ((1 << SMI_BWC_SCEN_CAM_PV) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_CAM_PV;
	else if ((1 << SMI_BWC_SCEN_CAM_CP) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_CAM_CP;
	else if ((1 << SMI_BWC_SCEN_VP_HIGH_RESOLUTION) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VP_HIGH_RESOLUTION;
	else if ((1 << SMI_BWC_SCEN_VP_HIGH_FPS) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VP_HIGH_FPS;
	else if ((1 << SMI_BWC_SCEN_VP) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VP;
	else if ((1 << SMI_BWC_SCEN_SWDEC_VP) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_SWDEC_VP;
	else if ((1 << SMI_BWC_SCEN_VENC) & u4Concurrency)
		eFinalScen = SMI_BWC_SCEN_VENC;
	else
		eFinalScen = SMI_BWC_SCEN_NORMAL;

	if (ePreviousFinalScen != eFinalScen) {
		ePreviousFinalScen = eFinalScen;
	} else {
		SMIMSG("Scen equal to %s, no need to change\n", smi_get_scenario_name(eFinalScen));
		spin_unlock(&g_SMIInfo.SMI_lock);
		smi_bus_optimization_clock_control(bus_optimization_sync, SMI_UNPREPARE_CLK);
		return 0;
	}

	smi_profile = eFinalScen;
	smi_bus_optimization_clock_control(bus_optimization_sync, SMI_ENABLE_CLK);
	smi_bus_optimization(bus_optimization_sync, eFinalScen);
	smi_bus_optimization_clock_control(bus_optimization_sync, SMI_DISABLE_CLK);
	SMIMSG("[SMI_PROFILE]=%s\n", smi_get_scenario_name(eFinalScen));


	spin_unlock(&g_SMIInfo.SMI_lock);
	smi_bus_optimization_clock_control(bus_optimization_sync, SMI_UNPREPARE_CLK);
	ovl_limit_uevent(smi_profile, g_smi_bwc_mm_info.hw_ovl_limit);

	return 0;
}

#if !defined(SMI_INTERNAL_CCF_SUPPORT)
struct larb_monitor larb_monitor_handler = {
	.level = LARB_MONITOR_LEVEL_HIGH,
	.backup = on_larb_power_off,
	.restore = on_larb_power_on
};
#endif				/* !defined(SMI_INTERNAL_CCF_SUPPORT) */

int smi_common_init(void)
{
	int i;

#if defined(SMI_INTERNAL_CCF_SUPPORT)
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	struct pg_callbacks *pold = 0;
#endif
#endif

	SMIMSG("Enter smi_common_init\n");

	if (!enable_bw_optimization) {
		SMIMSG("SMI enable_bw_optimization off\n");
		return 0;
	}

	for (i = 0; i < SMI_LARB_NUM; i++) {
		pLarbRegBackUp[i] = kmalloc(LARB_BACKUP_REG_SIZE, GFP_KERNEL | __GFP_ZERO);
		if (!pLarbRegBackUp[i])
			SMIERR("pLarbRegBackUp kmalloc fail %d\n", i);
	}


	/* apply init setting after kernel boot */
	smi_bus_optimization_clock_control(bus_optimization, SMI_PREPARE_CLK);
	smi_bus_optimization_clock_control(bus_optimization, SMI_ENABLE_CLK);

	/* apply mmu setting -- enable bit1 */
	smi_apply_mmu_setting();
	smi_apply_basic_setting();

	smi_bus_optimization(bus_optimization, SMI_BWC_SCEN_NORMAL);

	smi_bus_optimization_clock_control(bus_optimization, SMI_DISABLE_CLK);
	smi_bus_optimization_clock_control(bus_optimization, SMI_UNPREPARE_CLK);

#if defined(SMI_INTERNAL_CCF_SUPPORT)
	fglarbcallback = true;
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	pold = register_pg_callback(&smi_clk_subsys_handle);
	if (pold)
		SMIERR("smi reg clk cb call fail\n");
	else
		SMIMSG("smi reg clk cb call success\n");
#endif
#elif !defined(SMI_DUMMY)				/* !defined(SMI_INTERNAL_CCF_SUPPORT) */
	register_larb_monitor(&larb_monitor_handler);
#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */

	/*
	 * make sure all larb power is on before we register callback func.
	 * then, when larb power is first off, default register value will be backed up.
	 */


	/* After clock callback registration, it will restore incorrect value because backup is not called. */
	smi_first_restore = 0;
	return 0;

}

static int smi_open(struct inode *inode, struct file *file)
{
	file->private_data = kmalloc_array(SMI_BWC_SCEN_CNT, sizeof(unsigned int), GFP_ATOMIC);

	if (!file->private_data) {
		SMIMSG("Not enough entry for DDP open operation\n");
		return -ENOMEM;
	}

	memset(file->private_data, 0, SMI_BWC_SCEN_CNT * sizeof(unsigned int));

	return 0;
}

static int smi_release(struct inode *inode, struct file *file)
{

#if 0
	unsigned long u4Index = 0;
	unsigned long u4AssignCnt = 0;
	unsigned long *pu4Cnt = (unsigned long *)file->private_data;
	struct MTK_SMI_BWC_CONFIG config;

	for (; u4Index < SMI_BWC_SCEN_CNT; u4Index += 1) {
		if (pu4Cnt[u4Index]) {
			SMIMSG("Process:%s does not turn off BWC properly , force turn off %d\n",
			       current->comm, u4Index);
			u4AssignCnt = pu4Cnt[u4Index];
			config.b_on_off = 0;
			config.scenario = (enum MTK_SMI_BWC_SCEN) u4Index;
			do {
				smi_bwc_config(&config, pu4Cnt);
			} while (u4AssignCnt > 0);
		}
	}
#endif

	kfree(file->private_data);
	file->private_data = NULL;

	return 0;
}

static long smi_ioctl(struct file *pFile, unsigned int cmd, unsigned long param)
{
	int ret = 0;

	if (!enable_ioctl) {
		SMIMSG("SMI IOCTL disabled: cmd code=%d\n", cmd);
		return 0;
	}

	/* unsigned long * pu4Cnt = (unsigned long *)pFile->private_data; */

	switch (cmd) {

		/* disable reg access ioctl by default for possible security holes */
		/* TBD: check valid SMI register range */
	case MTK_IOC_SMI_BWC_CONFIG:{
			struct MTK_SMI_BWC_CONFIG cfg;

			ret = copy_from_user(&cfg, (void *)param, sizeof(struct MTK_SMI_BWC_CONFIG));
			if (ret) {
				SMIMSG(" SMI_BWC_CONFIG, copy_from_user failed: %d\n", ret);
				return -EFAULT;
			}

			SMIDBG(1, "before smi_bwc_config, smi_prepare_count=%d, smi_enable_count=%d\n",
				smi_prepare_count, smi_enable_count);
			ret = smi_bwc_config(&cfg, NULL);
			SMIDBG(1, "after smi_bwc_config, smi_prepare_count=%d, smi_enable_count=%d\n",
				smi_prepare_count, smi_enable_count);

			if (smi_prepare_count || smi_enable_count) {
				if (smi_debug_level > 99)
					SMIERR("clk status abnormal!!prepare or enable ref count is not 0\n");
				else
					SMIDBG(1, "clk status abnormal!!prepare or enable ref count is not 0\n");
			}

			break;
		}
		/* GMP start */
	case MTK_IOC_SMI_BWC_INFO_SET:{
			smi_set_mm_info_ioctl_wrapper(pFile, cmd, param);
			break;
		}
	case MTK_IOC_SMI_BWC_INFO_GET:{
			smi_get_mm_info_ioctl_wrapper(pFile, cmd, param);
			break;
		}
		/* GMP end */

	case MTK_IOC_SMI_DUMP_LARB:{
			unsigned int larb_index;

			ret = copy_from_user(&larb_index, (void *)param, sizeof(unsigned int));
			if (ret)
				return -EFAULT;

			smi_dumpLarb(larb_index);
		}
		break;

	case MTK_IOC_SMI_DUMP_COMMON:{
			unsigned int arg;

			ret = copy_from_user(&arg, (void *)param, sizeof(unsigned int));
			if (ret)
				return -EFAULT;

			smi_dumpCommon();
		}
		break;

#ifdef MMDVFS_HOOK
	case MTK_IOC_MMDVFS_CMD:
		{

			struct MTK_MMDVFS_CMD mmdvfs_cmd;

			if (disable_mmdvfs)
				return -EFAULT;

			if (copy_from_user(&mmdvfs_cmd, (void *)param, sizeof(struct MTK_MMDVFS_CMD)))
				return -EFAULT;


			mmdvfs_handle_cmd(&mmdvfs_cmd);

			if (copy_to_user
			    ((void *)param, (void *)&mmdvfs_cmd, sizeof(struct MTK_MMDVFS_CMD))) {
				return -EFAULT;
			}
		}
		break;
#endif
	default:
		return -1;
	}

	return ret;
}

static const struct file_operations smiFops = {
	.owner = THIS_MODULE,
	.open = smi_open,
	.release = smi_release,
	.unlocked_ioctl = smi_ioctl,
	.compat_ioctl = MTK_SMI_COMPAT_ioctl,
};

static dev_t smiDevNo = MKDEV(MTK_SMI_MAJOR_NUMBER, 0);
static inline int smi_register(void)
{
	if (alloc_chrdev_region(&smiDevNo, 0, 1, "MTK_SMI")) {
		SMIERR("Allocate device No. failed");
		return -EAGAIN;
	}
	/* Allocate driver */
	pSmiDev = cdev_alloc();

	if (!pSmiDev) {
		unregister_chrdev_region(smiDevNo, 1);
		SMIERR("Allocate mem for kobject failed");
		return -ENOMEM;
	}
	/* Attatch file operation. */
	cdev_init(pSmiDev, &smiFops);
	pSmiDev->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(pSmiDev, smiDevNo, 1)) {
		SMIERR("Attatch file operation failed");
		unregister_chrdev_region(smiDevNo, 1);
		return -EAGAIN;
	}

	return 0;
}

static unsigned long get_register_base(int i)
{
	unsigned long pa_value = 0;
	unsigned long va_value = 0;

	va_value = gSMIBaseAddrs[i];
	pa_value = virt_to_phys((void *)va_value);

	return pa_value;
}

void register_base_dump(void)
{
	int i = 0;

	for (i = 0; i < SMI_REG_REGION_MAX; i++) {
		SMIMSG("REG BASE:%s-->VA=0x%lx,PA=0x%lx\n",
		       smi_get_region_name(i), gSMIBaseAddrs[i], get_register_base(i));
	}
}

static struct class *pSmiClass;

/* MMDVFS related clk initialization */
static int smi_mmdvfs_clks_init(void)
{
#if defined(SMI_WHI) || defined(SMI_ALA) || defined(SMI_BIA)
		int i = 0;

		/* const int mmdvfs_disable_setting = disable_mmdvfs; */
		/* init clk mux of each MM clks*/
		for (i = 0; i < g_mmdvfs_adaptor->mmdvfs_clk_hw_maps_num; i++) {
			/* Get the clk mux desc */
			struct mmdvfs_clk_hw_map *hw_map_ptr = g_mmdvfs_adaptor->mmdvfs_clk_hw_maps + i;

			if (hw_map_ptr->config_method != MMDVFS_CLK_CONFIG_NONE) {
				SMIMSG("Init CLK %s\n", hw_map_ptr->clk_mux.ccf_name);
				hw_map_ptr->clk_mux.ccf_handle = get_smi_clk(hw_map_ptr->clk_mux.ccf_name);
			}
		}

		for (i = 0; i < g_mmdvfs_adaptor->mmdvfs_clk_sources_num; i++) {
			SMIMSG("Init CLK %s\n", g_mmdvfs_adaptor->mmdvfs_clk_sources[i].ccf_name);
			g_mmdvfs_adaptor->mmdvfs_clk_sources[i].ccf_handle =
			get_smi_clk(g_mmdvfs_adaptor->mmdvfs_clk_sources[i].ccf_name);
			}

		/* Enanle the MASK for CLK change */
		clk_mux_mask = 0xFFFF;

		SMIMSG("Finishe smi_mmdvfs_clks_init\n");

		/* Set default high berfore MMDVFS feature is enabled, */
		/* Onlye work when force_max_mmsys_clk is enabled */
		mmdvfs_default_start_delayed_setting();
#endif
		return 0;
}

static int smi_probe(struct platform_device *pdev)
{

	int i;

	static unsigned int smi_probe_cnt;
	struct device *smiDevice = NULL;
	int prev_smi_debug_level = smi_debug_level;

	smi_debug_level = 3;
	SMIMSG("Enter smi_probe\n");
	/* Debug only */
	if (smi_probe_cnt != 0) {
		SMIERR("Only support 1 SMI driver probed\n");
		return 0;
	}
	smi_probe_cnt++;
	SMIMSG("Allocate smi_dev space\n");
	smi_dev = kmalloc(sizeof(struct smi_device), GFP_KERNEL);

	if (!smi_dev) {
		SMIERR("Unable to allocate memory for smi driver\n");
		return -ENOMEM;
	}
	if (!pdev) {
		SMIERR("platform data missed\n");
		return -ENXIO;
	}
	/* Keep the device structure */
	smi_dev->dev = &pdev->dev;

	if (enable_bw_optimization) {
		/* Map registers */
		for (i = 0; i < SMI_REG_REGION_MAX; i++) {
			SMIMSG("Save region: %d\n", i);
			smi_dev->regs[i] = (void *)of_iomap(pdev->dev.of_node, i);

			if (!smi_dev->regs[i]) {
				SMIERR("Unable to ioremap registers, of_iomap fail, i=%d\n", i);
				return -ENOMEM;
			}
			/* Record the register base in global variable */
			gSMIBaseAddrs[i] = (unsigned long)(smi_dev->regs[i]);
			SMIMSG("DT, i=%d, region=%s, map_addr=0x%p, reg_pa=0x%lx\n", i,
			       smi_get_region_name(i), smi_dev->regs[i], get_register_base(i));
		}
#if defined(SMI_WHI)
		smi_dev->mmsys = (void *)of_iomap(pdev->dev.of_node, SMI_MMSYS_REG_INDX);
		if (!smi_dev->mmsys) {
			SMIERR("Unable to ioremap mmsys registers, of_iomap fail");
			return -ENOMEM;
		}
		mmsys_reg = (unsigned long) smi_dev->mmsys;
#endif
#if defined(SMI_INTERNAL_CCF_SUPPORT)
#if defined(SMI_WHI)
		smi_dev->smi_common_fifo0_clk = get_smi_clk("smi-common-FIFO0");
		smi_dev->smi_common_fifo1_clk = get_smi_clk("smi-common-FIFO1");
		smi_dev->smi_common_upsz0_clk = get_smi_clk("smi-common-UPSZ0");
		smi_dev->smi_common_upsz1_clk = get_smi_clk("smi-common-UPSZ1");
		smi_dev->smi_common_gals0_clk = get_smi_clk("smi-common-GALS0");
		smi_dev->smi_common_gals1_clk = get_smi_clk("smi-common-GALS1");
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_common_2x_clk = get_smi_clk("smi-common-2X");
		smi_dev->smi_larb0_clk = get_smi_clk("smi-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("smi-larb1");
		smi_dev->smi_larb2_clk = get_smi_clk("smi-larb2");
		smi_dev->smi_larb3_clk = get_smi_clk("smi-larb3");
		smi_dev->smi_larb4_vdec_clk = get_smi_clk("smi-larb4-vdec");
		smi_dev->smi_larb4_mm_clk = get_smi_clk("smi-larb4-mm");
		smi_dev->smi_larb5_img_clk = get_smi_clk("smi-larb5-img");
		smi_dev->smi_larb5_mm_clk = get_smi_clk("smi-larb5-mm");
		smi_dev->smi_larb6_cam_clk = get_smi_clk("smi-larb6-cam");
		smi_dev->smi_larb6_mm_clk = get_smi_clk("smi-larb6-mm");
		smi_dev->smi_larb7_venc0_clk = get_smi_clk("smi-larb7-venc-0");
		smi_dev->smi_larb7_venc1_clk = get_smi_clk("smi-larb7-venc-1");
		smi_dev->smi_larb7_venc2_clk = get_smi_clk("smi-larb7-venc-2");
		smi_dev->smi_larb7_mm_clk = get_smi_clk("smi-larb7-mm");
		smi_dev->smi_larb8_mjc0_clk = get_smi_clk("smi-larb8-mjc-0");
		smi_dev->smi_larb8_mjc1_clk = get_smi_clk("smi-larb8-mjc-1");
		smi_dev->smi_larb8_mm_clk = get_smi_clk("smi-larb8-mm");
		/* MTCMOS */
		smi_dev->mm_mtcmos = get_smi_clk("mtcmos-mm");
		smi_dev->img_mtcmos = get_smi_clk("mtcmos-img");
		smi_dev->cam_mtcmos = get_smi_clk("mtcmos-cam");
		smi_dev->vde_mtcmos = get_smi_clk("mtcmos-vde");
		smi_dev->ven_mtcmos = get_smi_clk("mtcmos-ven");
		smi_dev->mjc_mtcmos = get_smi_clk("mtcmos-mjc");
		smi_mmdvfs_clks_init();
#elif defined(SMI_ALA) || defined(SMI_VIN)
		smi_dev->smi_common_gals_comm0_clk = get_smi_clk("smi-common-gals-comm0");
		smi_dev->smi_common_gals_comm1_clk = get_smi_clk("smi-common-gals-comm1");
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_larb0_clk = get_smi_clk("mm-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("mm-larb1");
		smi_dev->smi_larb2_clk = get_smi_clk("img-larb2");
		smi_dev->gals_ipu2mm_clk = get_smi_clk("gals-ipu2mm");
		smi_dev->smi_larb5_clk = get_smi_clk("img-larb5");
		smi_dev->gals_img2mm_clk = get_smi_clk("gals-img2mm");
		smi_dev->smi_larb3_clk = get_smi_clk("cam-larb3");
		smi_dev->smi_larb6_clk = get_smi_clk("cam-larb6");
		smi_dev->gals_cam2mm_clk = get_smi_clk("gals-cam2mm");
		smi_dev->smi_larb4_clk = get_smi_clk("vdec-larb4");
		smi_dev->gals_vdec2mm_clk = get_smi_clk("gals-vdec2mm");
		smi_dev->smi_larb7_cke1venc_clk = get_smi_clk("venc-larb7-cke1venc");
		smi_dev->smi_larb7_jpgenc_clk = get_smi_clk("venc-larb7-jpgenc");
		smi_dev->gals_venc2mm_clk = get_smi_clk("gals-venc2mm");
		/* MTCMOS */
		smi_dev->mm_mtcmos = get_smi_clk("mtcmos-mm");
		smi_dev->img_mtcmos = get_smi_clk("mtcmos-isp");
		smi_dev->cam_mtcmos = get_smi_clk("mtcmos-cam");
		smi_dev->vde_mtcmos = get_smi_clk("mtcmos-vde");
		smi_dev->ven_mtcmos = get_smi_clk("mtcmos-ven");
		smi_mmdvfs_clks_init();
#elif defined(SMI_BIA)
		smi_dev->smi_common_gals_comm0_clk = get_smi_clk("smi-common-gals0");
		smi_dev->smi_common_gals_comm1_clk = get_smi_clk("smi-common-gals1");
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_larb0_clk = get_smi_clk("mm-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("img-larb1");
		smi_dev->smi_larb2_clk = get_smi_clk("cam-larb2");
		smi_dev->smi_larb3_clk_larb = get_smi_clk("venc-larb");
		smi_dev->smi_larb3_clk_venc = get_smi_clk("venc-venc");
		smi_dev->gals_cam2mm_clk = get_smi_clk("gals-cam2mm");
		/* MTCMOS */
		smi_dev->mm_mtcmos = get_smi_clk("mtcmos-mm");
		smi_dev->img_mtcmos = get_smi_clk("mtcmos-isp");
		smi_dev->cam_mtcmos = get_smi_clk("mtcmos-cam");
		smi_dev->ven_mtcmos = get_smi_clk("mtcmos-ven");
		smi_mmdvfs_clks_init();
#endif
#endif
	} else {
		SMIDBG(1, "enable_bw_optimization is disabled\n");
	}

	for (i = 0; i < SMI_LARB_NUM; i++)
		atomic_set(&(larbs_clock_count[i]), 0);

	SMIMSG("Execute smi_register\n");
	if (smi_register()) {
		dev_err(&pdev->dev, "register char failed\n");
		return -EAGAIN;
	}

	pSmiClass = class_create(THIS_MODULE, "MTK_SMI");
	if (IS_ERR(pSmiClass)) {
		int ret = PTR_ERR(pSmiClass);

		SMIERR("Unable to create class, err = %d", ret);
		return ret;
	}
	SMIMSG("Create device\n");
	smiDevice = device_create(pSmiClass, NULL, smiDevNo, NULL, "MTK_SMI");
	smiDeviceUevent = smiDevice;

	SMIMSG("SMI probe done.\n");

#if defined(SMI_WHI)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
	gLarbBaseAddr[2] = gSMIBaseAddrs[SMI_LARB2_REG_INDX];
	gLarbBaseAddr[3] = gSMIBaseAddrs[SMI_LARB3_REG_INDX];
	gLarbBaseAddr[4] = gSMIBaseAddrs[SMI_LARB4_REG_INDX];
	gLarbBaseAddr[5] = gSMIBaseAddrs[SMI_LARB5_REG_INDX];
	gLarbBaseAddr[6] = gSMIBaseAddrs[SMI_LARB6_REG_INDX];
	gLarbBaseAddr[7] = gSMIBaseAddrs[SMI_LARB7_REG_INDX];
	gLarbBaseAddr[8] = gSMIBaseAddrs[SMI_LARB8_REG_INDX];
#endif

	if (SMI_MMIT_PORTING) {
		/* for MMIT, we only write warb setting and return */
		SMIMSG("before setting, 0x100=0x%x", M4U_ReadReg32(get_common_base_addr(), 0x100));
		M4U_WriteReg32(get_common_base_addr(), 0x100, 0xb);
		SMIMSG("after setting, 0x100=0x%x", M4U_ReadReg32(get_common_base_addr(), 0x100));
		smi_debug_level = prev_smi_debug_level;
		return 0;
	}

	SMIMSG("Execute smi_common_init\n");
	SMIDBG(1, "before smi_common_init, smi_prepare_count=%d, smi_enable_count=%d\n",
	 smi_prepare_count, smi_enable_count);
	smi_common_init();
	SMIDBG(1, "after smi_common_init, smi_prepare_count=%d, smi_enable_count=%d\n",
	 smi_prepare_count, smi_enable_count);

	if (smi_prepare_count || smi_enable_count) {
		if (smi_debug_level > 99)
			SMIERR("clk status abnormal!!prepare or enable ref count is not 0\n");
		else
			SMIDBG(1, "clk status abnormal!!prepare or enable ref count is not 0\n");
	}

	smi_debug_level = prev_smi_debug_level;
	return 0;

}

char *smi_get_region_name(unsigned int region_indx)
{
	switch (region_indx) {
	case SMI_COMMON_REG_INDX:
		return "smi_common";
	case SMI_LARB0_REG_INDX:
		return "larb0";
	case SMI_LARB1_REG_INDX:
		return "larb1";
	case SMI_LARB2_REG_INDX:
		return "larb2";
	case SMI_LARB3_REG_INDX:
		return "larb3";
	case SMI_LARB4_REG_INDX:
		return "larb4";
	case SMI_LARB5_REG_INDX:
		return "larb5";
	case SMI_LARB6_REG_INDX:
		return "larb6";
	case SMI_LARB7_REG_INDX:
		return "larb7";
	case SMI_LARB8_REG_INDX:
		return "larb8";
	default:
		SMIMSG("invalid region id=%d", region_indx);
		return "unknown";
	}
}

static int smi_remove(struct platform_device *pdev)
{
	cdev_del(pSmiDev);
	unregister_chrdev_region(smiDevNo, 1);
	device_destroy(pSmiClass, smiDevNo);
	class_destroy(pSmiClass);
	return 0;
}

static int smi_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	/* Only work when force_max_mmsys_clk is enabled */
	mmdvfs_default_stop_delayed_setting();
	mmdvfs_default_step_set(MMDVFS_FINE_STEP_UNREQUEST);

	/* Only work when force_always_on_mm_clks_mask is enabled */
	mmdvfs_debug_set_mmdvfs_clks_enabled(0);

	return 0;
}

static int smi_resume(struct platform_device *pdev)
{
	/* Only work when force_always_on_mm_clks_mask is enabled */
	mmdvfs_debug_set_mmdvfs_clks_enabled(1);

	/* Only work when force_max_mmsys_clk is enabled */
	mmdvfs_default_step_set(MMDVFS_FINE_STEP_OPP0);
	return 0;
}

static const struct of_device_id smi_of_ids[] = {
	{.compatible = "mediatek,smi_common",},
	{}
};

static struct platform_driver smiDrv = {
	.probe = smi_probe,
	.remove = smi_remove,
	.suspend = smi_suspend,
	.resume = smi_resume,
	.driver = {
		   .name = "MTK_SMI",
		   .owner = THIS_MODULE,
		   .of_match_table = smi_of_ids,
		   }
};

static int __init smi_init(void)
{
	SMIMSG("smi_init enter\n");
	smi_driver_setting();
	spin_lock_init(&g_SMIInfo.SMI_lock);
#ifdef MMDVFS_HOOK
	mmdvfs_init(&g_smi_bwc_mm_info);
#endif
	memset(g_SMIInfo.pu4ConcurrencyTable, 0, SMI_BWC_SCEN_CNT * sizeof(unsigned int));

	/* Informs the kernel about the function to be called */
	/* if hardware matching MTK_SMI has been found */
	SMIMSG("register platform driver\n");
	if (platform_driver_register(&smiDrv)) {
		SMIERR("failed to register MAU driver");
		return -ENODEV;
	}
	SMIMSG("exit smi_init\n");
	return 0;
}

static void __exit smi_exit(void)
{
	platform_driver_unregister(&smiDrv);

}







void smi_client_status_change_notify(int module, int mode)
{

}

enum MTK_SMI_BWC_SCEN smi_get_current_profile(void)
{
	return (enum MTK_SMI_BWC_SCEN) smi_profile;
}
EXPORT_SYMBOL(smi_get_current_profile);
#if IS_ENABLED(CONFIG_COMPAT)
/* 32 bits process ioctl support: */
/* This is prepared for the future extension since currently the sizes of 32 bits */
/* and 64 bits smi parameters are the same. */

struct MTK_SMI_COMPAT_BWC_CONFIG {
	compat_int_t scenario;
	compat_int_t b_on_off;	/* 0 : exit this scenario , 1 : enter this scenario */
};

struct MTK_SMI_COMPAT_BWC_INFO_SET {
	compat_int_t property;
	compat_int_t value1;
	compat_int_t value2;
};

struct MTK_SMI_COMPAT_BWC_MM_INFO {
	compat_uint_t flag;	/* Reserved */
	compat_int_t concurrent_profile;
	compat_int_t sensor_size[2];
	compat_int_t video_record_size[2];
	compat_int_t display_size[2];
	compat_int_t tv_out_size[2];
	compat_int_t fps;
	compat_int_t video_encode_codec;
	compat_int_t video_decode_codec;
	compat_int_t hw_ovl_limit;
};

#define COMPAT_MTK_IOC_SMI_BWC_CONFIG      MTK_IOW(24, struct MTK_SMI_COMPAT_BWC_CONFIG)
#define COMPAT_MTK_IOC_SMI_BWC_INFO_SET    MTK_IOWR(28, struct MTK_SMI_COMPAT_BWC_INFO_SET)
#define COMPAT_MTK_IOC_SMI_BWC_INFO_GET    MTK_IOWR(29, struct MTK_SMI_COMPAT_BWC_MM_INFO)

static int compat_get_smi_bwc_config_struct(struct MTK_SMI_COMPAT_BWC_CONFIG __user *data32,
					    struct MTK_SMI_BWC_CONFIG __user *data)
{

	compat_int_t i;
	int err;

	/* since the int sizes of 32 A32 and A64 are equal so we don't convert them actually here */
	err = get_user(i, &(data32->scenario));
	err |= put_user(i, &(data->scenario));
	err |= get_user(i, &(data32->b_on_off));
	err |= put_user(i, &(data->b_on_off));

	return err;
}

static int compat_get_smi_bwc_mm_info_set_struct(struct MTK_SMI_COMPAT_BWC_INFO_SET __user *data32,
						 struct MTK_SMI_BWC_INFO_SET __user *data)
{

	compat_int_t i;
	int err;

	/* since the int sizes of 32 A32 and A64 are equal so we don't convert them actually here */
	err = get_user(i, &(data32->property));
	err |= put_user(i, &(data->property));
	err |= get_user(i, &(data32->value1));
	err |= put_user(i, &(data->value1));
	err |= get_user(i, &(data32->value2));
	err |= put_user(i, &(data->value2));

	return err;
}

static int compat_get_smi_bwc_mm_info_struct(struct MTK_SMI_COMPAT_BWC_MM_INFO __user *data32,
					     struct MTK_SMI_BWC_MM_INFO __user *data)
{
	compat_uint_t u;
	compat_int_t i;
	compat_int_t p[2];
	int err;

	/* since the int sizes of 32 A32 and A64 are equal so we don't convert them actually here */
	err = get_user(u, &(data32->flag));
	err |= put_user(u, &(data->flag));
	err |= get_user(i, &(data32->concurrent_profile));
	err |= put_user(i, &(data->concurrent_profile));
	err |= copy_from_user(p, &(data32->sensor_size), sizeof(p));
	err |= copy_to_user(&(data->sensor_size), p, sizeof(p));
	err |= copy_from_user(p, &(data32->video_record_size), sizeof(p));
	err |= copy_to_user(&(data->video_record_size), p, sizeof(p));
	err |= copy_from_user(p, &(data32->display_size), sizeof(p));
	err |= copy_to_user(&(data->display_size), p, sizeof(p));
	err |= copy_from_user(p, &(data32->tv_out_size), sizeof(p));
	err |= copy_to_user(&(data->tv_out_size), p, sizeof(p));
	err |= get_user(i, &(data32->fps));
	err |= put_user(i, &(data->fps));
	err |= get_user(i, &(data32->video_encode_codec));
	err |= put_user(i, &(data->video_encode_codec));
	err |= get_user(i, &(data32->video_decode_codec));
	err |= put_user(i, &(data->video_decode_codec));
	err |= get_user(i, &(data32->hw_ovl_limit));
	err |= put_user(i, &(data->hw_ovl_limit));

	return err;
}

static int compat_put_smi_bwc_mm_info_struct(struct MTK_SMI_COMPAT_BWC_MM_INFO __user *data32,
					     struct MTK_SMI_BWC_MM_INFO __user *data)
{

	compat_uint_t u;
	compat_int_t i;
	compat_int_t p[2];
	int err;

	/* since the int sizes of 32 A32 and A64 are equal so we don't convert them actually here */
	err = get_user(u, &(data->flag));
	err |= put_user(u, &(data32->flag));
	err |= get_user(i, &(data->concurrent_profile));
	err |= put_user(i, &(data32->concurrent_profile));
	err |= copy_from_user(p, &(data->sensor_size), sizeof(p));
	err |= copy_to_user(&(data32->sensor_size), p, sizeof(p));
	err |= copy_from_user(p, &(data->video_record_size), sizeof(p));
	err |= copy_to_user(&(data32->video_record_size), p, sizeof(p));
	err |= copy_from_user(p, &(data->display_size), sizeof(p));
	err |= copy_to_user(&(data32->display_size), p, sizeof(p));
	err |= copy_from_user(p, &(data->tv_out_size), sizeof(p));
	err |= copy_to_user(&(data32->tv_out_size), p, sizeof(p));
	err |= get_user(i, &(data->fps));
	err |= put_user(i, &(data32->fps));
	err |= get_user(i, &(data->video_encode_codec));
	err |= put_user(i, &(data32->video_encode_codec));
	err |= get_user(i, &(data->video_decode_codec));
	err |= put_user(i, &(data32->video_decode_codec));
	err |= get_user(i, &(data->hw_ovl_limit));
	err |= put_user(i, &(data32->hw_ovl_limit));
	return err;
}

static long MTK_SMI_COMPAT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_MTK_IOC_SMI_BWC_CONFIG:
		{
			if (COMPAT_MTK_IOC_SMI_BWC_CONFIG == MTK_IOC_SMI_BWC_CONFIG) {
				return filp->f_op->unlocked_ioctl(filp, cmd,
								  (unsigned long)compat_ptr(arg));
			} else {

				struct MTK_SMI_COMPAT_BWC_CONFIG __user *data32;
				struct MTK_SMI_BWC_CONFIG __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(struct MTK_SMI_BWC_CONFIG));

				if (!data)
					return -EFAULT;

				err = compat_get_smi_bwc_config_struct(data32, data);
				if (err)
					return err;

				ret = filp->f_op->unlocked_ioctl(filp, MTK_IOC_SMI_BWC_CONFIG,
								 (unsigned long)data);
				return ret;
			}
		}

	case COMPAT_MTK_IOC_SMI_BWC_INFO_SET:
		{

			if (COMPAT_MTK_IOC_SMI_BWC_INFO_SET == MTK_IOC_SMI_BWC_INFO_SET) {
				return filp->f_op->unlocked_ioctl(filp, cmd,
								  (unsigned long)compat_ptr(arg));
			} else {

				struct MTK_SMI_COMPAT_BWC_INFO_SET __user *data32;
				struct MTK_SMI_BWC_INFO_SET __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(struct MTK_SMI_BWC_INFO_SET));
				if (!data)
					return -EFAULT;

				err = compat_get_smi_bwc_mm_info_set_struct(data32, data);
				if (err)
					return err;

				return filp->f_op->unlocked_ioctl(filp, MTK_IOC_SMI_BWC_INFO_SET,
								  (unsigned long)data);
			}
		}
		/* Fall through */
	case COMPAT_MTK_IOC_SMI_BWC_INFO_GET:
		{

			if (COMPAT_MTK_IOC_SMI_BWC_INFO_GET == MTK_IOC_SMI_BWC_INFO_GET) {
				return filp->f_op->unlocked_ioctl(filp, cmd,
								  (unsigned long)compat_ptr(arg));
			} else {
				struct MTK_SMI_COMPAT_BWC_MM_INFO __user *data32;
				struct MTK_SMI_BWC_MM_INFO __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(struct MTK_SMI_BWC_MM_INFO));

				if (!data)
					return -EFAULT;

				err = compat_get_smi_bwc_mm_info_struct(data32, data);
				if (err)
					return err;

				ret = filp->f_op->unlocked_ioctl(filp, MTK_IOC_SMI_BWC_INFO_GET,
								 (unsigned long)data);

				err = compat_put_smi_bwc_mm_info_struct(data32, data);

				if (err)
					return err;

				return ret;
			}
		}

	case MTK_IOC_SMI_DUMP_LARB:
	case MTK_IOC_SMI_DUMP_COMMON:
	case MTK_IOC_MMDVFS_CMD:

		return filp->f_op->unlocked_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
	default:
		return -ENOIOCTLCMD;
	}

}

#endif

int get_mmdvfs_clk_mux_mask(void)
{
	return clk_mux_mask;
}

int is_mmdvfs_disabled(void)
{
	return disable_mmdvfs;
}


int is_mmdvfs_freq_hopping_disabled(void)
{
	return disable_freq_hopping;
}

int is_mmdvfs_freq_mux_disabled(void)
{
	return disable_freq_mux;
}

int is_force_max_mmsys_clk(void)
{
	return force_max_mmsys_clk;
}

int is_force_camera_hpm(void)
{
	return force_camera_hpm;
}

int force_always_on_mm_clks(void)
{
	return force_always_on_mm_clks_mask;
}

arch_initcall(smi_init);

static void smi_driver_setting(void)
{
#ifdef SMI_PARAM_BW_OPTIMIZATION
	enable_bw_optimization = SMI_PARAM_BW_OPTIMIZATION;
#endif

#ifdef SMI_PARAM_BUS_OPTIMIZATION
	bus_optimization = SMI_PARAM_BUS_OPTIMIZATION;
#endif

#ifdef SMI_PARAM_ENABLE_IOCTL
	enable_ioctl = SMI_PARAM_ENABLE_IOCTL;
#endif

#ifdef SMI_PARAM_DISABLE_FREQ_HOPPING
	disable_freq_hopping = SMI_PARAM_DISABLE_FREQ_HOPPING;
#endif

#ifdef SMI_PARAM_DISABLE_FREQ_MUX
	disable_freq_mux = SMI_PARAM_DISABLE_FREQ_MUX;
#endif

#ifdef SMI_PARAM_DISABLE_MMDVFS
	disable_mmdvfs = SMI_PARAM_DISABLE_MMDVFS;
#endif

#ifdef SMI_PARAM_DISABLE_FORCE_CAMERA_HPM
	force_camera_hpm = !(SMI_PARAM_DISABLE_FORCE_CAMERA_HPM);
#endif

#ifdef SMI_PARAM_DISABLE_FORCE_MMSYS_MAX_CLK
	force_max_mmsys_clk = !(SMI_PARAM_DISABLE_FORCE_MMSYS_MAX_CLK);
#endif

#ifdef SMI_PARAM_FORCE_MMSYS_CLKS_ALWAYS_ON
	force_always_on_mm_clks_mask = SMI_PARAM_FORCE_MMSYS_CLKS_ALWAYS_ON;
#endif

}

static void smi_apply_common_basic_setting(void)
{
#if defined(SMI_WHI)
	/* disable mmsys DCM for prevent memory transaction fail */
	SMIDBG(1, "before setting, DCM_val = 0x%x\n", M4U_ReadReg32(mmsys_reg, 0x130));
	M4U_WriteReg32(mmsys_reg, 0x134, 0x3fc0);
	SMIDBG(1, "after setting, DCM_val = 0x%x\n", M4U_ReadReg32(mmsys_reg, 0x130));
#endif
	SMIDBG(1, "start to apply common basic setting.\n");
	smi_common_setting(&smi_basic_setting_config);
	SMIDBG(1, "apply common basic setting done.\n");

}

static void smi_apply_larb_basic_setting(int larb)
{
	SMIDBG(1, "start to apply larb setting, larb=0x%x\n", larb);
	/* allow to manipulate mulitple larb at once */
	smi_larb_setting(larb, &smi_basic_setting_config);
	SMIDBG(1, "apply larb setting done.\n");
}

static void smi_apply_basic_setting(void)
{
	smi_apply_common_basic_setting();

	smi_apply_larb_basic_setting(bus_optimization);
}

static void smi_apply_larb_mmu_setting(int larb)
{
	/* allow to manipulate mulitple larb at once */
	int i = 0;
	int j = 0;
	unsigned int val = 0;
	struct SMI_SETTING *settings = &smi_mmu_setting_config;

	/* larb id is computed by bit, larb0 = 1 << 0... */
	/* set regs of larb */
	for (i = 0; i < SMI_LARB_NUM; i++) {
		int larb_mask = 1 << i;

		if (larb & larb_mask) {
			SMIDBG(1, "apply larb%d\n", i);
			for (j = 0; j < settings->smi_larb_reg_num[i]; j++) {
				SMIDBG(1, "before apply, offset:0x%x, register value=0x%x\n",
					settings->smi_larb_setting_vals[i][j].offset,
					M4U_ReadReg32(get_larb_base_addr(i),
				settings->smi_larb_setting_vals[i][j].offset));

				val = M4U_ReadReg32(get_larb_base_addr(i),
				settings->smi_larb_setting_vals[i][j].offset);
				/* we here set bit1 to 1 to enable cmd grouping capability */
				if (!val) {
					/* hw default setting is 1(VA), current get 0(PA)... */
					pr_err("hw default setting is 1(VA), current get 0(PA), it may cause error...\n");
				}
				val |= 0x2;

				M4U_WriteReg32(get_larb_base_addr(i),
				settings->smi_larb_setting_vals[i][j].offset, val);

				SMIDBG(1, "after apply, offset:0x%x, register value=0x%x\n",
					settings->smi_larb_setting_vals[i][j].offset,
					M4U_ReadReg32(get_larb_base_addr(i),
				settings->smi_larb_setting_vals[i][j].offset));
			}
			SMIDBG(1, "apply larb%d done\n", i);
		}
	}
}

static void smi_apply_mmu_setting(void)
{
	SMIDBG(1, "start to apply mmu setting.\n");
	smi_apply_larb_mmu_setting(bus_optimization);
	SMIDBG(1, "apply mmu setting done.\n");
}

void smi_common_ostd_setting(int enable)
{
	unsigned int val = 0;
	unsigned int tmp_val = 0;

	SMIDBG(1, "before setting, 0x118=0x%x, 0x11c=0x%x, 0x120=0x%x\n",
	M4U_ReadReg32(get_common_base_addr(), 0x118),
	M4U_ReadReg32(get_common_base_addr(), 0x11c),
	M4U_ReadReg32(get_common_base_addr(), 0x120));
	/* workaround for disable IPU/VENC/MJC write cmd via set write ostd = 0 */
	if (enable == 0) {
		val = 0xffcfbfff;
		tmp_val = M4U_ReadReg32(get_common_base_addr(), 0x118) & val;
		M4U_WriteReg32(get_common_base_addr(), 0x118, tmp_val);
		tmp_val = M4U_ReadReg32(get_common_base_addr(), 0x11c) & val;
		M4U_WriteReg32(get_common_base_addr(), 0x11c, tmp_val);
		tmp_val = M4U_ReadReg32(get_common_base_addr(), 0x120) & val;
		M4U_WriteReg32(get_common_base_addr(), 0x120, tmp_val);
	} else {
		val = 0x304000;
		tmp_val = M4U_ReadReg32(get_common_base_addr(), 0x118) | val;
		M4U_WriteReg32(get_common_base_addr(), 0x118, tmp_val);
		tmp_val = M4U_ReadReg32(get_common_base_addr(), 0x11c) | val;
		M4U_WriteReg32(get_common_base_addr(), 0x11c, tmp_val);
		tmp_val = M4U_ReadReg32(get_common_base_addr(), 0x120) | val;
		M4U_WriteReg32(get_common_base_addr(), 0x120, tmp_val);
	}
	SMIDBG(1, "after setting, 0x118=0x%x, 0x11c=0x%x, 0x120=0x%x\n",
	M4U_ReadReg32(get_common_base_addr(), 0x118),
	M4U_ReadReg32(get_common_base_addr(), 0x11c),
	M4U_ReadReg32(get_common_base_addr(), 0x120));
}

module_param_named(mmdvfs_rt_debug_disable_mask, mmdvfs_rt_debug_disable_mask, uint, S_IRUGO | S_IWUSR);
module_param_named(disable_mmdvfs, disable_mmdvfs, uint, S_IRUGO | S_IWUSR);
module_param_named(disable_freq_hopping, disable_freq_hopping, uint, S_IRUGO | S_IWUSR);
module_param_named(disable_freq_mux, disable_freq_mux, uint, S_IRUGO | S_IWUSR);
module_param_named(force_max_mmsys_clk, force_max_mmsys_clk, uint, S_IRUGO | S_IWUSR);
module_param_named(force_camera_hpm, force_camera_hpm, uint, S_IRUGO | S_IWUSR);
module_param_named(force_always_on_mm_clks_mask, force_always_on_mm_clks_mask, uint, S_IRUGO | S_IWUSR);
module_param_named(smi_debug_level, smi_debug_level, uint, S_IRUGO | S_IWUSR);
module_param_named(mmdvfs_debug_level, mmdvfs_debug_level, uint, S_IRUGO | S_IWUSR);
module_param_named(wifi_disp_transaction, wifi_disp_transaction, uint, S_IRUGO | S_IWUSR);
module_param_named(bus_optimization, bus_optimization, uint, S_IRUGO | S_IWUSR);
module_param_named(enable_ioctl, enable_ioctl, uint, S_IRUGO | S_IWUSR);
module_param_named(enable_bw_optimization, enable_bw_optimization, uint, S_IRUGO | S_IWUSR);
module_param_named(clk_mux_mask, clk_mux_mask, uint, S_IRUGO | S_IWUSR);
module_exit(smi_exit);

MODULE_DESCRIPTION("MTK SMI driver");
MODULE_AUTHOR("Kendrick Hsu<kendrick.hsu@mediatek.com>");
MODULE_LICENSE("GPL");
