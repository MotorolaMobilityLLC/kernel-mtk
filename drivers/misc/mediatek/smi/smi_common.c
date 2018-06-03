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
#if defined(SMI_D1)
#include "clk-mt6735-pg.h"
#elif defined(SMI_J)
#include "clk-mt6755-pg.h"
#elif defined(SMI_EV)
#include "clk-mt6797-pg.h"
#elif defined(SMI_OLY)
#include "clk-mt6757-pg.h"
#endif
/* notify clk is enabled/disabled for m4u*/
#include "m4u.h"
#else
#include <mach/mt_clkmgr.h>
#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */

#include <linux/io.h>

#include <linux/ioctl.h>
#include <linux/fs.h>

#if IS_ENABLED(CONFIG_COMPAT)
#include <linux/uaccess.h>
#include <linux/compat.h>
#endif

#include "mtk_smi.h"

#if defined(SMI_D1) || defined(SMI_D2) || defined(SMI_D3) || defined(SMI_J) ||  defined(SMI_EV) || defined(SMI_OLY)
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
unsigned long gLarbBaseAddr[SMI_LARB_NUM] = { 0 };

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

#if defined(SMI_D2)
#define SMI_REG_REGION_MAX 4

static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM
};


static unsigned short int larb0_port_backup[SMI_LARB0_PORT_NUM];
static unsigned short int larb1_port_backup[SMI_LARB1_PORT_NUM];
static unsigned short int larb2_port_backup[SMI_LARB2_PORT_NUM];

static unsigned short int *larb_port_backup[SMI_LARB_NUM] = {
	larb0_port_backup, larb1_port_backup, larb2_port_backup
};


#elif defined(SMI_D1)
#define SMI_REG_REGION_MAX 5

static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM
};

static unsigned short int larb0_port_backup[SMI_LARB0_PORT_NUM];
static unsigned short int larb1_port_backup[SMI_LARB1_PORT_NUM];
static unsigned short int larb2_port_backup[SMI_LARB2_PORT_NUM];
static unsigned short int larb3_port_backup[SMI_LARB3_PORT_NUM];
static unsigned short int *larb_port_backup[SMI_LARB_NUM] = {
	larb0_port_backup, larb1_port_backup, larb2_port_backup, larb3_port_backup
};


#elif defined(SMI_D3)
#define SMI_REG_REGION_MAX 5

static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM
};

static unsigned short int larb0_port_backup[SMI_LARB0_PORT_NUM];
static unsigned short int larb1_port_backup[SMI_LARB1_PORT_NUM];
static unsigned short int larb2_port_backup[SMI_LARB2_PORT_NUM];
static unsigned short int larb3_port_backup[SMI_LARB3_PORT_NUM];


static unsigned short int *larb_port_backup[SMI_LARB_NUM] = {
	larb0_port_backup, larb1_port_backup, larb2_port_backup, larb3_port_backup
};
#elif defined(SMI_R)

#define SMI_REG_REGION_MAX 3

static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM
};

static unsigned short int larb0_port_backup[SMI_LARB0_PORT_NUM];
static unsigned short int larb1_port_backup[SMI_LARB1_PORT_NUM];


static unsigned short int *larb_port_backup[SMI_LARB_NUM] = {
	larb0_port_backup, larb1_port_backup
};

#elif defined(SMI_J)
#define SMI_REG_REGION_MAX 5


static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM
};

static unsigned short int larb0_port_backup[SMI_LARB0_PORT_NUM];
static unsigned short int larb1_port_backup[SMI_LARB1_PORT_NUM];
static unsigned short int larb2_port_backup[SMI_LARB2_PORT_NUM];
static unsigned short int larb3_port_backup[SMI_LARB3_PORT_NUM];


static unsigned short int *larb_port_backup[SMI_LARB_NUM] = { larb0_port_backup,
	larb1_port_backup, larb2_port_backup, larb3_port_backup
};

#elif defined(SMI_EV)
#define SMI_REG_REGION_MAX 8


static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM, SMI_LARB4_PORT_NUM,
	SMI_LARB5_PORT_NUM, SMI_LARB6_PORT_NUM
};

static unsigned short int larb0_port_backup[SMI_LARB0_PORT_NUM];
static unsigned short int larb1_port_backup[SMI_LARB1_PORT_NUM];
static unsigned short int larb2_port_backup[SMI_LARB2_PORT_NUM];
static unsigned short int larb3_port_backup[SMI_LARB3_PORT_NUM];
static unsigned short int larb4_port_backup[SMI_LARB4_PORT_NUM];
static unsigned short int larb5_port_backup[SMI_LARB5_PORT_NUM];
static unsigned short int larb6_port_backup[SMI_LARB6_PORT_NUM];


static unsigned short int *larb_port_backup[SMI_LARB_NUM] = { larb0_port_backup,
	larb1_port_backup, larb2_port_backup, larb3_port_backup,
	larb4_port_backup, larb5_port_backup, larb6_port_backup
};

#elif defined(SMI_OLY)
#define SMI_REG_REGION_MAX 7


static const unsigned int larb_port_num[SMI_LARB_NUM] = { SMI_LARB0_PORT_NUM,
	SMI_LARB1_PORT_NUM, SMI_LARB2_PORT_NUM, SMI_LARB3_PORT_NUM, SMI_LARB4_PORT_NUM,
	SMI_LARB5_PORT_NUM
};

#elif defined(SMI_DUMMY)
#define SMI_REG_REGION_MAX 1

static const unsigned int larb_port_num[SMI_LARB_NUM] = {0};
static unsigned short int *larb_port_backup[SMI_LARB_NUM] = {0};

#endif

static unsigned long gSMIBaseAddrs[SMI_REG_REGION_MAX];

/* SMI COMMON register list to be backuped */
#if defined(SMI_EV)
#define SMI_COMMON_BACKUP_REG_NUM   13
static unsigned short g_smi_common_backup_reg_offset[SMI_COMMON_BACKUP_REG_NUM] = { 0x100, 0x104,
	0x108, 0x10c, 0x110, 0x114, 0x118, 0x11c, 0x220, 0x230, 0x234, 0x238, 0x300
};
#elif defined(SMI_OLY)
/* g_smi_common_backup_reg_offset is not used anymore */
#else
#define SMI_COMMON_BACKUP_REG_NUM   8
static unsigned short g_smi_common_backup_reg_offset[SMI_COMMON_BACKUP_REG_NUM] = { 0x100, 0x104,
	0x108, 0x10c, 0x110, 0x230, 0x234, 0x300
};
#endif

#if !defined(SMI_OLY)
static unsigned int g_smi_common_backup[SMI_COMMON_BACKUP_REG_NUM];
#endif

struct smi_device {
	struct device *dev;
	void __iomem *regs[SMI_REG_REGION_MAX];
#if defined(SMI_INTERNAL_CCF_SUPPORT)
	struct clk *smi_common_clk;
	struct clk *smi_larb0_clk;
	struct clk *smi_larb2_clk;
	struct clk *smi_larb1_clk;
	struct clk *smi_larb3_larb_clk;
	struct clk *smi_larb3_venc_clk;
	struct clk *smi_larb4_clk;
	/* MJC series is for EVEREST */
	struct clk *smi_larb4_mjc_clk;
	struct clk *smi_larb4_mm_clk;
	struct clk *smi_larb4_mjc_smi_larb_clk;
	struct clk *smi_larb4_mjc_top_clk_0_clk;
	struct clk *smi_larb4_mjc_top_clk_1_clk;
	struct clk *smi_larb4_mjc_top_clk_2_clk;
	struct clk *smi_larb4_mjc_larb4_asif_clk;
	struct clk *smi_larb5_clk;
	struct clk *smi_larb6_clk;
	struct clk *larb0_mtcmos;
	struct clk *larb1_mtcmos;
	struct clk *larb2_mtcmos;
	struct clk *larb3_mtcmos;
	struct clk *larb4_mtcmos;
	struct clk *larb5_mtcmos;
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


static unsigned int *pLarbRegBackUp[SMI_LARB_NUM];
#if !defined(SMI_OLY)
static int g_bInited;
#endif

MTK_SMI_BWC_MM_INFO g_smi_bwc_mm_info = {
	0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 0, 0, 0,
	SF_HWC_PIXEL_MAX_NORMAL
};

char *smi_port_name[][21] = {
	{			/* 0 MMSYS */
	 "disp_ovl0", "disp_rdma0", "disp_rdma1", "disp_wdma0", "disp_ovl1",
	 "disp_rdma2", "disp_wdma1", "disp_od_r", "disp_od_w", "mdp_rdma0",
	 "mdp_rdma1", "mdp_wdma", "mdp_wrot0", "mdp_wrot1"},
	{ /* 1 VDEC */ "hw_vdec_mc_ext", "hw_vdec_pp_ext", "hw_vdec_ufo_ext", "hw_vdec_vld_ext",
	 "hw_vdec_vld2_ext", "hw_vdec_avc_mv_ext", "hw_vdec_pred_rd_ext",
	 "hw_vdec_pred_wr_ext", "hw_vdec_ppwrap_ext"},
	{ /* 2 ISP */ "imgo", "rrzo", "aao", "lcso", "esfko", "imgo_d", "lsci", "lsci_d", "bpci",
	 "bpci_d", "ufdi", "imgi", "img2o", "img3o", "vipi", "vip2i", "vip3i",
	 "lcei", "rb", "rp", "wr"},
	{ /* 3 VENC */ "venc_rcpu", "venc_rec", "venc_bsdma", "venc_sv_comv", "venc_rd_comv",
	 "jpgenc_bsdma", "remdc_sdma", "remdc_bsdma", "jpgenc_rdma", "jpgenc_sdma",
	 "jpgdec_wdma", "jpgdec_bsdma", "venc_cur_luma", "venc_cur_chroma",
	 "venc_ref_luma", "venc_ref_chroma", "remdc_wdma", "venc_nbm_rdma",
	 "venc_nbm_wdma"},
	{ /* 4 MJC */ "mjc_mv_rd", "mjc_mv_wr", "mjc_dma_rd", "mjc_dma_wr"}
};

enum smi_clk_operation {
	SMI_PREPARE_CLK,
	SMI_ENABLE_CLK,
	SMI_DISABLE_CLK,
	SMI_UNPREPARE_CLK
};

static unsigned long get_register_base(int i);
static void smi_driver_setting(void);
static char *smi_get_scenario_name(MTK_SMI_BWC_SCEN scen);
static void smi_bus_optimization_clock_control(int optimization_larbs, enum smi_clk_operation oper);
#if defined(SMI_OLY)
static void smi_apply_common_basic_setting(void);
static void smi_apply_larb_basic_setting(int larb);
static void smi_apply_basic_setting(void);
static void smi_apply_larb_mmu_setting(int larb);
static void smi_apply_mmu_setting(void);
#endif

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
	else
		return gLarbBaseAddr[larb_id];

}

unsigned long get_common_base_addr(void)
{
	return gSMIBaseAddrs[SMI_COMMON_REG_INDX];
}
EXPORT_SYMBOL(get_common_base_addr);


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

#if !defined(CONFIG_MTK_FPGA) && !defined(CONFIG_FPGA_EARLY_PORTING)
static void smi_prepare_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		int ret = 0;

		ret = clk_prepare(smi_clk);
		if (ret) {
			SMIMSG("clk_prepare return error %d, %s\n", ret, name);
		} else {
			SMIDBG(3, "clk:%s prepare done.\n", name);
			SMIDBG(3, "smi_prepare_count=%d\n", ++smi_prepare_count);
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
			SMIDBG(3, "clk:%s enable done.\n", name);
			SMIDBG(3, "smi_enable_count=%d\n", ++smi_enable_count);
		}
	} else {
		SMIMSG("clk_enable error, smi_clk can't be NULL, %s\n", name);
	}
}

static void smi_unprepare_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		clk_unprepare(smi_clk);
		SMIDBG(3, "clk:%s unprepare done.\n", name);
		SMIDBG(3, "smi_prepare_count=%d\n", --smi_prepare_count);
	} else {
		SMIMSG("smi_unprepare error, smi_clk can't be NULL, %s\n", name);
	}
}

static void smi_disable_clk(struct clk *smi_clk, char *name)
{
	if (smi_clk) {
		clk_disable(smi_clk);
		SMIDBG(3, "clk:%s disable done.\n", name);
		SMIDBG(3, "smi_enable_count=%d\n", --smi_enable_count);
	} else {
		SMIMSG("smi_disable error, smi_clk can't be NULL, %s\n", name);
	}
}

#endif /* !defined(CONFIG_MTK_FPGA) && !defined(CONFIG_FPGA_EARLY_PORTING) */
/* end MTCMOS*/
#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */

static int larb_clock_enable(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_MTK_FPGA) && !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(SMI_DUMMY)
	char name[30];

	sprintf(name, "larb%d", larb_id);


	switch (larb_id) {
#if !defined(SMI_INTERNAL_CCF_SUPPORT)
	case 0:
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
		enable_clock(MT_CG_DISP0_SMI_LARB0, name);
		break;
	case 1:
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
#if defined(SMI_R)
		enable_clock(MT_CG_LARB1_SMI_CKPDN, name);
#else
		enable_clock(MT_CG_VDEC1_LARB, name);
#endif
		break;
	case 2:
#if !defined(SMI_R)
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
		enable_clock(MT_CG_IMAGE_LARB2_SMI, name);
#endif
		break;
	case 3:
		enable_clock(MT_CG_DISP0_SMI_COMMON, name);
#if defined(SMI_D1)
		enable_clock(MT_CG_VENC_LARB, name);
#elif defined(SMI_D3)
		enable_clock(MT_CG_VENC_VENC, name);
#endif
		break;
#else
	case 0:
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		break;
	case 1:
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_enable_clk(smi_dev->larb1_mtcmos, "larb1_mtcmos");
		}
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		break;
	case 2:
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_enable_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
		}
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		break;
	case 3:
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_enable_clk(smi_dev->larb3_mtcmos, "larb3_mtcmos");
		}
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
#if defined(SMI_OLY)
		smi_enable_clk(smi_dev->smi_larb3_venc_clk, "smi_larb3_venc_clk");
#endif
		smi_enable_clk(smi_dev->smi_larb3_larb_clk, "smi_larb3_larb_clk");
		break;
	case 4:
#if defined(SMI_EV)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_enable_clk(smi_dev->larb4_mtcmos, "larb4_mtcmos");
		}
		smi_enable_clk(smi_dev->smi_larb4_mjc_top_clk_1_clk, "smi_larb4_mjc_top_clk_1_clk");
		smi_enable_clk(smi_dev->smi_larb4_mjc_top_clk_2_clk, "smi_larb4_mjc_top_clk_2_clk");
		smi_enable_clk(smi_dev->smi_larb4_mjc_larb4_asif_clk, "smi_larb4_mjc_larb4_asif_clk");
#elif defined(SMI_OLY)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
#endif
		break;
	case 5:
#if defined(SMI_EV)
		if (enable_mtcmos)
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
#elif defined(SMI_OLY)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_enable_clk(smi_dev->larb5_mtcmos, "larb5_mtcmos");
		}
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
#endif
		break;
	case 6:
#if defined(SMI_EV)
		if (enable_mtcmos) {
			smi_enable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_enable_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
		}
		smi_enable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_enable_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
#endif
		break;
#endif
	default:
		break;
	}
#endif
	if (larb_id < SMI_LARB_NUM)
		atomic_inc(&(larbs_clock_count[larb_id]));
	return 0;
}

static int larb_clock_prepare(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_MTK_FPGA) && !defined(CONFIG_FPGA_EARLY_PORTING) && defined(SMI_INTERNAL_CCF_SUPPORT)
	char name[30];

	sprintf(name, "larb%d", larb_id);

	switch (larb_id) {
	case 0:
		/* must enable MTCOMS before clk */
		/* common MTCMOS is called with larb0_MTCMOS */
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		break;
	case 1:
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_prepare_clk(smi_dev->larb1_mtcmos, "larb1_mtcmos");
		}
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		break;
	case 2:
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_prepare_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
		}
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		break;
	case 3:
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_prepare_clk(smi_dev->larb3_mtcmos, "larb3_mtcmos");
		}
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
#if defined(SMI_OLY)
		smi_prepare_clk(smi_dev->smi_larb3_venc_clk, "smi_larb3_venc_clk");
#endif
		smi_prepare_clk(smi_dev->smi_larb3_larb_clk, "smi_larb3_larb_clk");
		break;
	case 4:
#if defined(SMI_EV)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_prepare_clk(smi_dev->larb4_mtcmos, "larb4_mtcmos");
		}
		smi_prepare_clk(smi_dev->smi_larb4_mjc_top_clk_1_clk, "smi_larb4_mjc_top_clk_1_clk");
		smi_prepare_clk(smi_dev->smi_larb4_mjc_top_clk_2_clk, "smi_larb4_mjc_top_clk_2_clk");
		smi_prepare_clk(smi_dev->smi_larb4_mjc_larb4_asif_clk, "smi_larb4_mjc_larb4_asif_clk");
#elif defined(SMI_OLY)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
#endif
		break;
	case 5:
#if defined(SMI_EV)
		if (enable_mtcmos)
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
#elif defined(SMI_OLY)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_prepare_clk(smi_dev->larb5_mtcmos, "larb5_mtcmos");
		}
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
#endif
		break;
	case 6:
#if defined(SMI_EV)
		if (enable_mtcmos) {
			smi_prepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
			smi_prepare_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
		}
		smi_prepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		smi_prepare_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		break;
#endif
		break;
	default:
		break;
	}
#endif
	return 0;
}

static int larb_clock_disable(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_MTK_FPGA) && !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(SMI_DUMMY)
	char name[30];

	sprintf(name, "larb%d", larb_id);

	switch (larb_id) {
#if !defined(SMI_INTERNAL_CCF_SUPPORT)
	case 0:
		disable_clock(MT_CG_DISP0_SMI_LARB0, name);
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
		break;
	case 1:
#if defined(SMI_R)
		disable_clock(MT_CG_LARB1_SMI_CKPDN, name);
#else
		disable_clock(MT_CG_VDEC1_LARB, name);
#endif
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
		break;
	case 2:
#if !defined(SMI_R)
		disable_clock(MT_CG_IMAGE_LARB2_SMI, name);
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
#endif
		break;
	case 3:
#if defined(SMI_D1)
		disable_clock(MT_CG_VENC_LARB, name);
#elif defined(SMI_D3)
		disable_clock(MT_CG_VENC_VENC, name);
#endif
		disable_clock(MT_CG_DISP0_SMI_COMMON, name);
		break;
#else
	case 0:
		smi_disable_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		break;
	case 1:
		smi_disable_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->larb1_mtcmos, "larb1_mtcmos");
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
	case 2:
		smi_disable_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
	case 3:
		smi_disable_clk(smi_dev->smi_larb3_larb_clk, "smi_larb3_larb_clk");
#if defined(SMI_OLY)
		smi_disable_clk(smi_dev->smi_larb3_venc_clk, "smi_larb3_venc_clk");
#endif
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->larb3_mtcmos, "larb3_mtcmos");
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
	case 4:
#if defined(SMI_EV)
		smi_disable_clk(smi_dev->smi_larb4_mjc_larb4_asif_clk, "smi_larb4_mjc_larb4_asif_clk");
		smi_disable_clk(smi_dev->smi_larb4_mjc_top_clk_2_clk, "smi_larb4_mjc_top_clk_2_clk");
		smi_disable_clk(smi_dev->smi_larb4_mjc_top_clk_1_clk, "smi_larb4_mjc_top_clk_1_clk");
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->larb4_mtcmos, "larb4_mtcmos");
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
#elif defined(SMI_OLY)
		smi_disable_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
#endif
		break;
	case 5:
#if defined(SMI_EV)
		smi_disable_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos)
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
#elif defined(SMI_OLY)
		smi_disable_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->larb5_mtcmos, "larb5_mtcmos");
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
#endif
		break;
	case 6:
#if defined(SMI_EV)
		smi_disable_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		smi_disable_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_disable_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
			smi_disable_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
#endif
		break;
#endif
	default:
		break;
	}
#endif
	if (larb_id < SMI_LARB_NUM)
		atomic_dec(&(larbs_clock_count[larb_id]));
	return 0;
}

static int larb_clock_unprepare(int larb_id, int enable_mtcmos)
{
#if !defined(CONFIG_MTK_FPGA) && !defined(CONFIG_FPGA_EARLY_PORTING) && defined(SMI_INTERNAL_CCF_SUPPORT)
	char name[30];

	sprintf(name, "larb%d", larb_id);

	switch (larb_id) {
	case 0:
		/* must enable MTCOMS before clk */
		/* common MTCMOS is called with larb0_MTCMOS */
		smi_unprepare_clk(smi_dev->smi_larb0_clk, "smi_larb0_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		break;
	case 1:
		smi_unprepare_clk(smi_dev->smi_larb1_clk, "smi_larb1_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->larb1_mtcmos, "larb1_mtcmos");
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
	case 2:
		smi_unprepare_clk(smi_dev->smi_larb2_clk, "smi_larb2_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
	case 3:
		smi_unprepare_clk(smi_dev->smi_larb3_larb_clk, "smi_larb3_larb_clk");
#if defined(SMI_OLY)
		smi_unprepare_clk(smi_dev->smi_larb3_venc_clk, "smi_larb3_venc_clk");
#endif
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->larb3_mtcmos, "larb3_mtcmos");
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
		break;
	case 4:
#if defined(SMI_EV)
		smi_unprepare_clk(smi_dev->smi_larb4_mjc_larb4_asif_clk, "smi_larb4_mjc_larb4_asif_clk");
		smi_unprepare_clk(smi_dev->smi_larb4_mjc_top_clk_2_clk, "smi_larb4_mjc_top_clk_2_clk");
		smi_unprepare_clk(smi_dev->smi_larb4_mjc_top_clk_1_clk, "smi_larb4_mjc_top_clk_1_clk");
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->larb4_mtcmos, "larb4_mtcmos");
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
#elif defined(SMI_OLY)
		smi_unprepare_clk(smi_dev->smi_larb4_clk, "smi_larb4_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
#endif
		break;
	case 5:
#if defined(SMI_EV)
		smi_unprepare_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos)
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
#elif defined(SMI_OLY)
		smi_unprepare_clk(smi_dev->smi_larb5_clk, "smi_larb5_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->larb5_mtcmos, "larb5_mtcmos");
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
#endif
		break;
	case 6:
#if defined(SMI_EV)
		smi_unprepare_clk(smi_dev->smi_larb6_clk, "smi_larb6_clk");
		smi_unprepare_clk(smi_dev->smi_common_clk, "smi_common_clk");
		if (enable_mtcmos) {
			smi_unprepare_clk(smi_dev->larb2_mtcmos, "larb2_mtcmos");
			smi_unprepare_clk(smi_dev->larb0_mtcmos, "larb0_mtcmos");
		}
#endif
		break;
	default:
		break;
	}
#endif
	return 0;
}
#if !defined(SMI_OLY)
static void backup_smi_common(void)
{
	int i;
	int err_count = 0;

	for (i = 0; i < SMI_COMMON_BACKUP_REG_NUM; i++) {
		g_smi_common_backup[i] = M4U_ReadReg32(get_common_base_addr(), (unsigned long)
						       g_smi_common_backup_reg_offset[i]);
		if (!g_smi_common_backup[i])
			err_count++;
	}

	if (err_count)
		SMIMSG("backup fail!!\n");
}
static void restore_smi_common(void)
{
	int err_count = 0;
	int i = 0;

	for (i = 0; i < SMI_COMMON_BACKUP_REG_NUM; i++) {
		if (!g_smi_common_backup[i])
			err_count++;
	}

	if (err_count)
		SMIMSG("backup value abnormal!!\n");

	if (smi_debug_level > 0) {
		SMIMSG("smi_profile=%d, dump before setting\n", smi_profile);
		smi_dumpCommonDebugMsg(0);
	}

	smi_common_setting(smi_profile_config[smi_profile].setting);

	if (smi_debug_level > 0) {
		SMIMSG("dump after setting\n");
		smi_dumpCommonDebugMsg(0);
	}

	if (!M4U_ReadReg32(get_common_base_addr(), (unsigned long)
						       g_smi_common_backup_reg_offset[0]))
		SMIMSG("restore fail!!\n");

}
static void backup_larb_smi(int index)
{
	int port_index = 0;
	unsigned short int *backup_ptr = NULL;
	unsigned long larb_base = 0;
	unsigned long larb_offset = 0x200;
	int total_port_num = 0;

	/* boundary check for larb_port_num and larb_port_backup access */
	if (index < 0 || index >= SMI_LARB_NUM)
		return;

	larb_base = gLarbBaseAddr[index];
	total_port_num = larb_port_num[index];
	backup_ptr = larb_port_backup[index];

	/* boundary check for port value access */
	if (total_port_num <= 0 || !backup_ptr)
		return;

	for (port_index = 0; port_index < total_port_num; port_index++) {
		*backup_ptr = (unsigned short int)(M4U_ReadReg32(larb_base, larb_offset));
		backup_ptr++;
		larb_offset += 4;
	}

	/* backup smi common along with larb0, smi common clk is guaranteed to be on when processing larbs */
	if (index == 0)
		backup_smi_common();
}
static void restore_larb_smi(int index)
{
	int port_index = 0;
	int i = 0;
	unsigned short int *backup_ptr = NULL;
	unsigned long larb_base = 0;
	unsigned long larb_offset = 0x200;
	unsigned int backup_value = 0;
	int total_port_num = 0;

	/* boundary check for larb_port_num and larb_port_backup access */
	if (index < 0 || index >= SMI_LARB_NUM)
		return;

	larb_base = gLarbBaseAddr[index];
	total_port_num = larb_port_num[index];
	backup_ptr = larb_port_backup[index];

	/* boundary check for port value access */
	if (total_port_num <= 0 || !backup_ptr)
		return;

	/* restore smi common along with larb0, smi common clk is guaranteed to be on when processing larbs */
	if (index == 0)
		restore_smi_common();

	for (port_index = 0; port_index < total_port_num; port_index++) {
		backup_value = *backup_ptr;
		M4U_WriteReg32(larb_base, larb_offset, backup_value);
		backup_ptr++;
		larb_offset += 4;
	}

	/* set grouping */
	if (smi_restore_num[index]) {
		for (i = 0 ; i < smi_restore_num[index]; i++)
			M4U_WriteReg32(larb_base, smi_larb_restore[index][i].offset, smi_larb_restore[index][i].value);
	}

	/* we do not backup 0x20 because it is a fixed setting */
	M4U_WriteReg32(larb_base, 0x20, smi_vc_setting[index].value);

	/* turn off EMI empty OSTD dobule, fixed setting */
	M4U_WriteReg32(larb_base, 0x2c, 4);

}
#endif
static int larb_reg_backup(int larb)
{
#if defined(SMI_OLY)
	/* do nothing, only apply setting in restore */
#else
	unsigned int *pReg = pLarbRegBackUp[larb];
	unsigned long larb_base = gLarbBaseAddr[larb];

	*(pReg++) = M4U_ReadReg32(larb_base, SMI_LARB_CON);

	backup_larb_smi(larb);

	if (larb == 0)
		g_bInited = 0;
#endif
	return 0;
}
#if !defined(SMI_OLY)
static int smi_larb_init(unsigned int larb)
{
	unsigned int regval = 0;
	unsigned int regval1 = 0;
	unsigned int regval2 = 0;
	unsigned long larb_base = get_larb_base_addr(larb);

	/* Clock manager enable LARB clock before call back restore already,
	 * it will be disabled after restore call back returns
	 * Got to enable OSTD before engine starts
	 */
	regval = M4U_ReadReg32(larb_base, SMI_LARB_STAT);

	/* TODO: FIX ME */
	/* regval1 = M4U_ReadReg32(larb_base , SMI_LARB_MON_BUS_REQ0); */
	/* regval2 = M4U_ReadReg32(larb_base , SMI_LARB_MON_BUS_REQ1); */

	if (regval == 0) {
		SMIDBG(1, "Init OSTD for larb_base: 0x%lx\n", larb_base);
		M4U_WriteReg32(larb_base, SMI_LARB_OSTDL_SOFT_EN, 0xffffffff);
	} else {
		SMIMSG("Larb: 0x%lx is busy : 0x%x , port:0x%x,0x%x ,fail to set OSTD\n",
		       larb_base, regval, regval1, regval2);
		if (smi_debug_level >= 1) {
			SMIERR("DISP_MDP LARB  0x%lx OSTD cannot be set:0x%x,port:0x%x,0x%x\n",
			       larb_base, regval, regval1, regval2);
		} else {
			dump_stack();
		}
	}

	restore_larb_smi(larb);

	return 0;
}
#endif
int larb_reg_restore(int larb)
{
#if defined(SMI_OLY)
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
#else
	unsigned long larb_base = SMI_ERROR_ADDR;
	unsigned int regval = 0;
	unsigned int *pReg = NULL;

	larb_base = get_larb_base_addr(larb);

	/* The larb assign doesn't exist */
	if (larb_base == SMI_ERROR_ADDR) {
		SMIMSG("Can't find the base address for Larb%d\n", larb);
		return 0;
	}

	if (larb >= SMI_LARB_NUM || larb < 0) {
		SMIMSG("Can't find the backup register value for Larb%d\n", larb);
		return 0;
	}

	pReg = pLarbRegBackUp[larb];

	SMIDBG(1, "+larb_reg_restore(), larb_idx=%d\n", larb);
	SMIDBG(1, "m4u part restore, larb_idx=%d\n", larb);
	/* warning: larb_con is controlled by set/clr */

	regval = (smi_first_restore) ? M4U_ReadReg32(larb_base, SMI_LARB_CON) : *(pReg++);

	M4U_WriteReg32(larb_base, SMI_LARB_CON_CLR, ~(regval));
	M4U_WriteReg32(larb_base, SMI_LARB_CON_SET, (regval));

	smi_larb_init(larb);
#endif
	return 0;
}

/* callback after larb clock is enabled */
#if !defined(SMI_INTERNAL_CCF_SUPPORT)
void on_larb_power_on(struct larb_monitor *h, int larb_idx)
{
	larb_reg_restore(larb_idx);
}

/* callback before larb clock is disabled */
void on_larb_power_off(struct larb_monitor *h, int larb_idx)
{
	larb_reg_backup(larb_idx);
}
#endif				/* !defined(SMI_INTERNAL_CCF_SUPPORT) */

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

void on_larb_power_off_with_ccf(int larb_idx)
{
	if (larb_idx < 0 || larb_idx >= SMI_LARB_NUM) {
		SMIMSG("incorrect larb:%d\n", larb_idx);
		return;
	}
	/* enable clk here for get correct register value */
	larb_clock_prepare(larb_idx, 0);
	larb_clock_enable(larb_idx, 0);
	larb_reg_backup(larb_idx);
	larb_clock_disable(larb_idx, 0);
	larb_clock_unprepare(larb_idx, 0);
}
#endif				/* defined(SMI_INTERNAL_CCF_SUPPORT) */




/* Fake mode check, e.g. WFD */
static int fake_mode_handling(MTK_SMI_BWC_CONFIG *p_conf, unsigned int *pu4LocalCnt)
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

	switch (sys) {
	case SYS_DIS:
		i4larbid |= 1 << 0;
#if defined(SMI_EV)
		/* 5 is disp */
		i4larbid |= 1 << 5;
#elif defined(SMI_OLY)
		/* 4 is disp */
		i4larbid |= 1 << 4;
#endif
		break;
	case SYS_VDE:
		i4larbid |= 1 << 1;
		break;
	case SYS_ISP:
#if defined(SMI_OLY)
		i4larbid |= 1 << 5;
#else
		i4larbid |= 1 << 2;
#endif
#if defined(SMI_EV)
		/* 6 is ISP */
		i4larbid |= 1 << 6;
#endif
		break;
	case SYS_VEN:
		i4larbid |= 1 << 3;
	break;
#if defined(SMI_EV)
	case SYS_MJC:
		i4larbid |= 1 << 4;
		break;
#endif
#if defined(SMI_OLY)
	case SYS_CAM:
		i4larbid |= 1 << 2;
		break;
#endif
	default:
		i4larbid = 0;
		break;
	}
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
#if defined(SMI_D1)
					/* inform m4u to restore register value */
					m4u_larb_backup((int)i4larbid);
#endif
				}
		}
		i4larbid = i4larbid >> 1;
		i++;
	} while (i4larbid != 0);
}

static void smiclk_subsys_before_off(enum subsys_id sys)
{
	unsigned int i4larbid = smiclk_subsys_2_larb(sys);
	int i = 0;

	if (!fglarbcallback) {
		SMIDBG(1, "don't need backup incb\n");
		return;
	}

	do {
		if ((i4larbid & 1) && (1 << i & bus_optimization)) {
			if (i < SMI_LARB_NUM) {
				SMIDBG(1, "ready to call backup with larb%d.\n", i);
				on_larb_power_off_with_ccf(i);
#if defined(SMI_D1)
					/* inform m4u to backup register value */
					m4u_larb_restore((int)i4larbid);
#endif
					}

		}
		i4larbid = i4larbid >> 1;
		i++;
	} while (i4larbid != 0);
}

struct pg_callbacks smi_clk_subsys_handle = {
	.before_off = smiclk_subsys_before_off,
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
			larb_clock_prepare(i, 1);
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
			larb_clock_unprepare(i, 1);
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
			larb_clock_enable(i, 1);
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
			larb_clock_disable(i, 1);
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
static char *smi_get_scenario_name(MTK_SMI_BWC_SCEN scen)
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
	default:
		return "unknown scenario";
	}
	return "";
}
static int smi_bwc_config(MTK_SMI_BWC_CONFIG *p_conf, unsigned int *pu4LocalCnt)
{
	int i;
	int result = 0;
	unsigned int u4Concurrency = 0;
	int bus_optimization_sync = bus_optimization;
	MTK_SMI_BWC_SCEN eFinalScen;
	static MTK_SMI_BWC_SCEN ePreviousFinalScen = SMI_BWC_SCEN_CNT;

	if ((p_conf->scenario >= SMI_BWC_SCEN_CNT) || (p_conf->scenario < 0)) {
		SMIERR("Incorrect SMI BWC config : 0x%x, how could this be...\n", p_conf->scenario);
		return -1;
	}

	SMIMSG("current request is turn %s %d\n", p_conf->b_on_off ? "on" : "off", p_conf->scenario);
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

	SMIMSG("after update, u4Concurrency=0x%x\n", u4Concurrency);
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
	SMIMSG("[SMI_PROFILE]: %s\n", smi_get_scenario_name(eFinalScen));


	spin_unlock(&g_SMIInfo.SMI_lock);
	smi_bus_optimization_clock_control(bus_optimization_sync, SMI_UNPREPARE_CLK);
	ovl_limit_uevent(smi_profile, g_smi_bwc_mm_info.hw_ovl_limit);

	/*
	 * force 30 fps in VR slow motion, because disp driver set fps apis got mutex,
	 * call these APIs only when necessary
	 */
	{
		static unsigned int current_fps;

		if ((eFinalScen == SMI_BWC_SCEN_VR_SLOW) && (current_fps != 30)) {
			/* force 30 fps in VR slow motion profile */
			primary_display_force_set_vsync_fps(30);
			current_fps = 30;
			SMIMSG("[SMI_PROFILE] set 30 fps\n");
		} else if ((eFinalScen != SMI_BWC_SCEN_VR_SLOW) && (current_fps == 30)) {
			/* back to normal fps */
			current_fps = primary_display_get_fps();
			primary_display_force_set_vsync_fps(current_fps);
			SMIMSG("[SMI_PROFILE] back to %u fps\n", current_fps);
		}
	}
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
#if defined(DEBUG_TEST)
	int cur_bus_optimization;
#endif
#if defined(SMI_INTERNAL_CCF_SUPPORT)
	struct pg_callbacks *pold = 0;
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

#if defined(DEBUG_TEST)
	SMIMSG("before test, bus_optimization=0x%x\n", bus_optimization);
	cur_bus_optimization = bus_optimization;
	for (i = 0; i < SMI_LARB_NUM; i++) {
		SMIMSG("test larb%d enable clock\n", i);
		bus_optimization = 1 << i;
		smi_bus_optimization_clock_control(bus_optimization, SMI_PREPARE_CLK);
		smi_bus_optimization_clock_control(bus_optimization, SMI_ENABLE_CLK);

		smi_apply_mmu_setting();
		smi_apply_basic_setting();
		smi_bus_optimization(bus_optimization, SMI_BWC_SCEN_NORMAL);

		smi_bus_optimization_clock_control(bus_optimization, SMI_DISABLE_CLK);
		smi_bus_optimization_clock_control(bus_optimization, SMI_UNPREPARE_CLK);
		bus_optimization = 0;
	}
	bus_optimization = cur_bus_optimization;
	SMIMSG("after test, bus_optimization=0x%x\n", bus_optimization);
#endif
	/* apply init setting after kernel boot */
	smi_bus_optimization_clock_control(bus_optimization, SMI_PREPARE_CLK);
	smi_bus_optimization_clock_control(bus_optimization, SMI_ENABLE_CLK);

	/* apply mmu setting -- enable bit1 */
#if defined(SMI_OLY)
	smi_apply_mmu_setting();
	smi_apply_basic_setting();
#endif
	smi_bus_optimization(bus_optimization, SMI_BWC_SCEN_NORMAL);

	smi_bus_optimization_clock_control(bus_optimization, SMI_DISABLE_CLK);
	smi_bus_optimization_clock_control(bus_optimization, SMI_UNPREPARE_CLK);

#if defined(SMI_INTERNAL_CCF_SUPPORT)
	fglarbcallback = true;

	pold = register_pg_callback(&smi_clk_subsys_handle);
	if (pold)
		SMIERR("smi reg clk cb call fail\n");
	else
		SMIMSG("smi reg clk cb call success\n");

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
	MTK_SMI_BWC_CONFIG config;

	for (; u4Index < SMI_BWC_SCEN_CNT; u4Index += 1) {
		if (pu4Cnt[u4Index]) {
			SMIMSG("Process:%s does not turn off BWC properly , force turn off %d\n",
			       current->comm, u4Index);
			u4AssignCnt = pu4Cnt[u4Index];
			config.b_on_off = 0;
			config.scenario = (MTK_SMI_BWC_SCEN) u4Index;
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
			MTK_SMI_BWC_CONFIG cfg;

			ret = copy_from_user(&cfg, (void *)param, sizeof(MTK_SMI_BWC_CONFIG));
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

			MTK_MMDVFS_CMD mmdvfs_cmd;

			if (disable_mmdvfs)
				return -EFAULT;

			if (copy_from_user(&mmdvfs_cmd, (void *)param, sizeof(MTK_MMDVFS_CMD)))
				return -EFAULT;


			mmdvfs_handle_cmd(&mmdvfs_cmd);

			if (copy_to_user
			    ((void *)param, (void *)&mmdvfs_cmd, sizeof(MTK_MMDVFS_CMD))) {
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

static int smi_probe(struct platform_device *pdev)
{

	int i;

	static unsigned int smi_probe_cnt;
	struct device *smiDevice = NULL;
	int prev_smi_debug_level = smi_debug_level;

	smi_debug_level = 1;
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

#if defined(SMI_INTERNAL_CCF_SUPPORT)
#if defined(SMI_D1)
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_larb0_clk = get_smi_clk("smi-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("vdec1-larb");
		smi_dev->smi_larb2_clk = get_smi_clk("img-larb2");
		smi_dev->smi_larb3_larb_clk = get_smi_clk("venc-venc");
#elif defined(SMI_J)
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_larb0_clk = get_smi_clk("smi-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("vdec1-larb");
		smi_dev->smi_larb2_clk = get_smi_clk("img-larb2");
		smi_dev->smi_larb3_larb_clk = get_smi_clk("venc-venc");
#elif defined(SMI_EV)
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_larb0_clk = get_smi_clk("smi-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("vdec-larb1");
		smi_dev->smi_larb2_clk = get_smi_clk("img-larb2");
		smi_dev->smi_larb3_larb_clk = get_smi_clk("venc-larb3");
		smi_dev->smi_larb4_mjc_clk = get_smi_clk("mjc-larb4");
		smi_dev->smi_larb4_mm_clk = get_smi_clk("mm-larb4");
		smi_dev->smi_larb4_mjc_smi_larb_clk = get_smi_clk("mjc_smi_larb");
		smi_dev->smi_larb4_mjc_top_clk_0_clk = get_smi_clk("mjc_top_clk_0");
		smi_dev->smi_larb4_mjc_top_clk_1_clk = get_smi_clk("mjc_top_clk_1");
		smi_dev->smi_larb4_mjc_top_clk_2_clk = get_smi_clk("mjc_top_clk_2");
		smi_dev->smi_larb4_mjc_larb4_asif_clk = get_smi_clk("mjc_larb4_asif");
		smi_dev->smi_larb5_clk = get_smi_clk("smi-larb5");
		smi_dev->smi_larb6_clk = get_smi_clk("img2-larb6");
#elif defined(SMI_OLY)
		smi_dev->smi_common_clk = get_smi_clk("smi-common");
		smi_dev->smi_larb0_clk = get_smi_clk("smi-larb0");
		smi_dev->smi_larb1_clk = get_smi_clk("smi-larb1");
		smi_dev->smi_larb2_clk = get_smi_clk("smi-larb2");
		smi_dev->smi_larb3_larb_clk = get_smi_clk("smi-larb3");
		smi_dev->smi_larb3_venc_clk = get_smi_clk("smi-larb3-2");
		smi_dev->smi_larb4_clk = get_smi_clk("smi-larb4");
		smi_dev->smi_larb5_clk = get_smi_clk("smi-larb5");
#endif
		/* MTCMOS */
		smi_dev->larb0_mtcmos = get_smi_clk("mtcmos-dis");
		smi_dev->larb1_mtcmos = get_smi_clk("mtcmos-vde");
#if defined(SMI_OLY)
		smi_dev->larb2_mtcmos = get_smi_clk("mtcmos-cam");
#else
		smi_dev->larb2_mtcmos = get_smi_clk("mtcmos-isp");
#endif
		smi_dev->larb3_mtcmos = get_smi_clk("mtcmos-ven");
#if defined(SMI_EV)
		smi_dev->larb4_mtcmos = get_smi_clk("mtcmos-mjc");
#endif
#if defined(SMI_OLY)
		smi_dev->larb5_mtcmos = get_smi_clk("mtcmos-isp");
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
#if defined(SMI_D2)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
	gLarbBaseAddr[2] = gSMIBaseAddrs[SMI_LARB2_REG_INDX];
#elif defined(SMI_D1) || defined(SMI_D3)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
	gLarbBaseAddr[2] = gSMIBaseAddrs[SMI_LARB2_REG_INDX];
	gLarbBaseAddr[3] = gSMIBaseAddrs[SMI_LARB3_REG_INDX];
#elif defined(SMI_J)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
	gLarbBaseAddr[2] = gSMIBaseAddrs[SMI_LARB2_REG_INDX];
	gLarbBaseAddr[3] = gSMIBaseAddrs[SMI_LARB3_REG_INDX];
#elif defined(SMI_R)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
#elif defined(SMI_EV)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
	gLarbBaseAddr[2] = gSMIBaseAddrs[SMI_LARB2_REG_INDX];
	gLarbBaseAddr[3] = gSMIBaseAddrs[SMI_LARB3_REG_INDX];
	gLarbBaseAddr[4] = gSMIBaseAddrs[SMI_LARB4_REG_INDX];
	gLarbBaseAddr[5] = gSMIBaseAddrs[SMI_LARB5_REG_INDX];
	gLarbBaseAddr[6] = gSMIBaseAddrs[SMI_LARB6_REG_INDX];

#elif defined(SMI_OLY)
	gLarbBaseAddr[0] = gSMIBaseAddrs[SMI_LARB0_REG_INDX];
	gLarbBaseAddr[1] = gSMIBaseAddrs[SMI_LARB1_REG_INDX];
	gLarbBaseAddr[2] = gSMIBaseAddrs[SMI_LARB2_REG_INDX];
	gLarbBaseAddr[3] = gSMIBaseAddrs[SMI_LARB3_REG_INDX];
	gLarbBaseAddr[4] = gSMIBaseAddrs[SMI_LARB4_REG_INDX];
	gLarbBaseAddr[5] = gSMIBaseAddrs[SMI_LARB5_REG_INDX];
#endif

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
	return 0;
}

static int smi_resume(struct platform_device *pdev)
{
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

MTK_SMI_BWC_SCEN smi_get_current_profile(void)
{
	return (MTK_SMI_BWC_SCEN) smi_profile;
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
					    MTK_SMI_BWC_CONFIG __user *data)
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
						 MTK_SMI_BWC_INFO_SET __user *data)
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
					     MTK_SMI_BWC_MM_INFO __user *data)
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
					     MTK_SMI_BWC_MM_INFO __user *data)
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
				MTK_SMI_BWC_CONFIG __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(MTK_SMI_BWC_CONFIG));

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
				MTK_SMI_BWC_INFO_SET __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(MTK_SMI_BWC_INFO_SET));
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
				MTK_SMI_BWC_MM_INFO __user *data;
				int err;

				data32 = compat_ptr(arg);
				data = compat_alloc_user_space(sizeof(MTK_SMI_BWC_MM_INFO));

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

int smi_bus_enable(enum SMI_MASTER_ID master_id, char *user_name)
{
	SMIMSG("smi_bus_enable is not support in this platform\n");
	return -1;
}

int smi_bus_disable(enum SMI_MASTER_ID master_id, char *user_name)
{
	SMIMSG("smi_bus_disable is not support in this platform\n");
	return -1;
}

int smi_clk_prepare(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIMSG("smi_clk_prepare is not support in this platform\n");
	return -1;
}

int smi_clk_enable(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIMSG("smi_clk_enable is not support in this platform\n");
	return -1;
}

int smi_clk_unprepare(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIMSG("smi_clk_unprepare is not support in this platform\n");
	return -1;
}

int smi_clk_disable(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos)
{
	SMIMSG("smi_clk_disable is not support in this platform\n");
	return -1;
}

subsys_initcall(smi_init);

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

}
#if defined(SMI_OLY)
static void smi_apply_common_basic_setting(void)
{
	smi_common_setting(&smi_basic_setting_config);
}

static void smi_apply_larb_basic_setting(int larb)
{
	/* allow to manipulate mulitple larb at once */
	smi_larb_setting(larb, &smi_basic_setting_config);
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
		}
	}
}

static void smi_apply_mmu_setting(void)
{
	smi_apply_larb_mmu_setting(bus_optimization);
}

#endif
module_param_named(disable_mmdvfs, disable_mmdvfs, uint, S_IRUGO | S_IWUSR);
module_param_named(disable_freq_hopping, disable_freq_hopping, uint, S_IRUGO | S_IWUSR);
module_param_named(disable_freq_mux, disable_freq_mux, uint, S_IRUGO | S_IWUSR);
module_param_named(force_max_mmsys_clk, force_max_mmsys_clk, uint, S_IRUGO | S_IWUSR);
module_param_named(force_camera_hpm, force_camera_hpm, uint, S_IRUGO | S_IWUSR);
module_param_named(smi_debug_level, smi_debug_level, uint, S_IRUGO | S_IWUSR);
module_param_named(wifi_disp_transaction, wifi_disp_transaction, uint, S_IRUGO | S_IWUSR);
module_param_named(bus_optimization, bus_optimization, uint, S_IRUGO | S_IWUSR);
module_param_named(enable_ioctl, enable_ioctl, uint, S_IRUGO | S_IWUSR);
module_param_named(enable_bw_optimization, enable_bw_optimization, uint, S_IRUGO | S_IWUSR);

module_exit(smi_exit);

MODULE_DESCRIPTION("MTK SMI driver");
MODULE_AUTHOR("Kendrick Hsu<kendrick.hsu@mediatek.com>");
MODULE_LICENSE("GPL");
