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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/list.h>
#ifdef CONFIG_MTK_AEE_FEATURE
#include <mt-plat/aee.h>
#endif
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <mt-plat/mt_device_apc.h>
#include <mt-plat/sync_write.h>
#include "mach/irqs.h"
#include <mt-plat/dma.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <mt-plat/mt_io.h>
#include "mach/emi_mpu.h"

#define ENABLE_EMI_CHKER
#define ENABLE_EMI_WATCH_POINT

#define NR_REGION_ABORT 15
#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define TIMEOUT 100
#define AXI_VIO_MONITOR_TIME    (1 * HZ)

static struct work_struct emi_mpu_work;
static struct workqueue_struct *emi_mpu_workqueue;

static unsigned int vio_addr;
static unsigned int emi_physical_offset;

struct mst_tbl_entry {
	u32 master;
	u32 port;
	u32 id_mask;
	u32 id_val;
	char *note;
	char *name;
};

struct emi_mpu_notifier_block {
	struct list_head list;
	emi_mpu_notifier notifier;
};


static const struct mst_tbl_entry mst_tbl[] = {
	/* apmcu */
#if defined(CONFIG_ARCH_MT6753)
	/* --cluster 0 Write */
	{
		.master = MST_ID_APMCU_0,
		.port = 0x0,
		.id_mask = 0x3E7,
		.id_val = 0x004,
		.note = "CA53: Core nn system",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_1,
		.port = 0x0,
		.id_mask = 0x3E7,
		.id_val = 0x024,
		.note = "CA53: Core nn barrier",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_2,
		.port = 0x0,
		.id_mask = 0x3FF,
		.id_val = 0x044,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_3,
		.port = 0x0,
		.id_mask = 0x3FF,
		.id_val = 0x04C,
		.note = "CA53: SCU generated barrier",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_4,
		.port = 0x0,
		.id_mask = 0x3F7,
		.id_val = 0x054,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_5,
		.port = 0x0,
		.id_mask = 0x3E7,
		.id_val = 0x064,
		.note = "CA53: Core nn non-re-orderable device write",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_6,
		.port = 0x0,
		.id_mask = 0x387,
		.id_val = 0x084,
		.note =
		"CA53: Write to normal memory, or re-orderable device memory",
		.name = "CA53"
	},
	/*--cluster 0 Read */
	{
		.master = MST_ID_APMCU_7,
		.port = 0x0,
		.id_mask = 0x39F,
		.id_val = 0x084,
		.note = "CA53: ACP read",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_8,
		.port = 0x0,
		.id_mask = 0x39F,
		.id_val = 0x08C,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_9,
		.port = 0x0,
		.id_mask = 0x397,
		.id_val = 0x094,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_10,
		.port = 0x0,
		.id_mask = 0x307,
		.id_val = 0x104,
		.note = "CA53: Core nn read",
		.name = "CA53"
	},
	/*--cluster 1 Write*/
	{
		.master = MST_ID_APMCU_11,
		.port = 0x0,
		.id_mask = 0x3E7,
		.id_val = 0x003,
		.note = "CA53: Core nn system",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_12,
		.port = 0x0,
		.id_mask = 0x3E7,
		.id_val = 0x023,
		.note = "CA53: Core nn barrier",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_13,
		.port = 0x0,
		.id_mask = 0x3FF,
		.id_val = 0x043,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_14,
		.port = 0x0,
		.id_mask = 0x3FF,
		.id_val = 0x04B,
		.note = "CA53: SCU generated barrier",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_15,
		.port = 0x0,
		.id_mask = 0x3F7,
		.id_val = 0x053,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_16,
		.port = 0x0,
		.id_mask = 0x3E7,
		.id_val = 0x063,
		.note = "CA53: Core nn non-re-orderable device write",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_17,
		.port = 0x0,
		.id_mask = 0x387,
		.id_val = 0x083,
		.note =
		"CA53: Write to normal memory, or re-orderable device memory",
		.name = "CA53"
	},
	/*--cluster 1 Read*/
	{
		.master = MST_ID_APMCU_18,
		.port = 0x0,
		.id_mask = 0x39F,
		.id_val = 0x083,
		.note = "CA53: ACP read",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_19,
		.port = 0x0,
		.id_mask = 0x39F,
		.id_val = 0x08B,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_20,
		.port = 0x0,
		.id_mask = 0x397,
		.id_val = 0x093,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_21,
		.port = 0x0,
		.id_mask = 0x307,
		.id_val = 0x103,
		.note = "CA53: Core nn read",
		.name = "CA53"
	},
#else
	{
		.master = MST_ID_APMCU_0,
		.port = 0x0,
		.id_mask = 0x3FC,
		.id_val = 0x000,
		.note = "CA53: Core nn system",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_1,
		.port = 0x0,
		.id_mask = 0x3FC,
		.id_val = 0x004,
		.note = "CA53: Core nn barrier",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_2,
		.port = 0x0,
		.id_mask = 0x3FF,
		.id_val = 0x008,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_3,
		.port = 0x0,
		.id_mask = 0x3FF,
		.id_val = 0x009,
		.note = "CA53: SCU generated barrier",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_4,
		.port = 0x0,
		.id_mask = 0x3FE,
		.id_val = 0x006,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_5,
		.port = 0x0,
		.id_mask = 0x3FC,
		.id_val = 0x00C,
		.note = "CA53: Core nn non-re-orderable device write",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_6,
		.port = 0x0,
		.id_mask = 0x3F0,
		.id_val = 0x010,
		.note =
		"CA53: Write to normal memory, or re-orderable device memory",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_7,
		.port = 0x0,
		.id_mask = 0x3F3,
		.id_val = 0x010,
		.note = "CA53: ACP read",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_8,
		.port = 0x0,
		.id_mask = 0x3F3,
		.id_val = 0x011,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_9,
		.port = 0x0,
		.id_mask = 0x3F2,
		.id_val = 0x012,
		.note = "CA53: Unused",
		.name = "CA53"
	},
	{
		.master = MST_ID_APMCU_10,
		.port = 0x0,
		.id_mask = 0x3E0,
		.id_val = 0x020,
		.note = "CA53: Core nn read",
		.name = "CA53"
	},
#endif
	/* MM */
	{
		.master = MST_ID_MM_0,
		.port = 0x1,
		.id_mask = 0x380,
		.id_val = 0x000,
		.note = "MM: Larb0",
		.name = "MM_Larb0"
	},
	{
		.master = MST_ID_MM_1,
		.port = 0x1,
		.id_mask = 0x380,
		.id_val = 0x080,
		.note = "MM: Larb1",
		.name = "MM_Larb1"
	},
	{
		.master = MST_ID_MM_2,
		.port = 0x1,
		.id_mask = 0x380,
		.id_val = 0x100,
		.note = "MM: Larb2",
		.name = "MM_Larb2"
	},
	{
		.master = MST_ID_MM_3,
		.port = 0x1,
		.id_mask = 0x380,
		.id_val = 0x180,
		.note = "MM: Larb3",
		.name = "MM_Larb3"
	},
	{
		.master = MST_ID_MM_4,
		.port = 0x1,
		.id_mask = 0x3FE,
		.id_val = 0x3FC,
		.note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU"
	},

	/* Periperal */
	{
		.master = MST_ID_PERI_0,
		.port = 0x2,
		.id_mask = 0x3F7,
		.id_val = 0x000,
		.note = "PERI: DebugTop",
		.name = "DebugTop"
	},
	{
		.master = MST_ID_PERI_1,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x002,
		.note = "PERI: MSDC0",
		.name = "MSDC0"
	},
	{
		.master = MST_ID_PERI_2,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x001,
		.note = "PERI: reserve",
		.name = "reserve"
	},
	{
		.master = MST_ID_PERI_3,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x021,
		.note = "PERI: reserve",
		.name = "reserve"
	},
	{
		.master = MST_ID_PERI_4,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x041,
		.note = "PERI: MSDC3",
		.name = "MSDC3"
	},
	{
		.master = MST_ID_PERI_5,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x061,
		.note = "PERI: USB2.0",
		.name = "USB"
	},
	{
		.master = MST_ID_PERI_6,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x009,
		.note = "PERI: PWM",
		.name = "PWM"
	},
	{
		.master = MST_ID_PERI_7,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x029,
		.note = "PERI: MSDC1",
		.name = "MSDC1"
	},
	{
		.master = MST_ID_PERI_8,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x049,
		.note = "PERI: MSDC2",
		.name = "MSDC2"
	},
	{
		.master = MST_ID_PERI_9,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x069,
		.note = "PERI: SPI0",
		.name = "SPI0"
	},
	{
		.master = MST_ID_PERI_10,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x011,
		.note = "PERI: MD1 + MD2",
		.name = "MD1_MD2"
	},
	{
		.master = MST_ID_PERI_11,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x031,
		.note = "PERI: SPM",
		.name = "SPM"
	},
	{
		.master = MST_ID_PERI_12,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x051,
		.note = "PERI: AUDIO",
		.name = "AUDIO"
	},
	{
		.master = MST_ID_PERI_13,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x071,
		.note = "PERI: THERM",
		.name = "THERM"
	},
	{
		.master = MST_ID_PERI_14,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x019,
		.note = "PERI: DMA",
		.name = "DMA"
	},
	{
		.master = MST_ID_PERI_15,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x003,
		.note = "PERI: CONNSYS",
		.name = "CONNSYS"
	},
	{
		.master = MST_ID_PERI_16,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x004,
		.note = "PERI: GCPU_M",
		.name = "GCPU_M"
	},
	{
		.master = MST_ID_PERI_17,
		.port = 0x2,
		.id_mask = 0x3E7,
		.id_val = 0x005,
		.note = "PERI: GCE_M",
		.name = "GCE_M"
	},
	{
		.master = MST_ID_PERI_18,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x006,
		.note = "PERI: CLDMA_AP",
		.name = "CLDMA_AP"
	},
	{
		.master = MST_ID_PERI_19,
		.port = 0x2,
		.id_mask = 0x3FF,
		.id_val = 0x00E,
		.note = "PERI: CLDMA_MD",
		.name = "CLDMA_MD"
	},

	/* Modem */
	{
		.master = MST_ID_MDMCU_0,
		.port = 0x3,
		.id_mask = 0x30F,
		.id_val = 0x000,
		.note = "MDMCU: ARM7",
		.name = "MDMCU"
	},
	{
		.master = MST_ID_MDMCU_1,
		.port = 0x3,
		.id_mask = 0x3FF,
		.id_val = 0x002,
		.note = "MDMCU",
		.name = "MDMCU"
	},
	{
		.master = MST_ID_MDMCU_2,
		.port = 0x3,
		.id_mask = 0x30F,
		.id_val = 0x004,
		.note = "MDMCU: PREFETCH",
		.name = "MDMCU"
	},
	{
		.master = MST_ID_MDMCU_3,
		.port = 0x3,
		.id_mask = 0x30F,
		.id_val = 0x008,
		.note = "MDMCU: ALC",
		.name = "MDMCU"
	},
	{
		.master = MST_ID_C2KMCU_0,
		.port = 0x3,
		.id_mask = 0x3FF,
		.id_val = 0x003,
		.note = "MDMCU: CK2 MCUP+ HW:ARM D PORT",
		.name = "MDMCU_C2K"
	},
	{
		.master = MST_ID_C2KMCU_1,
		.port = 0x3,
		.id_mask = 0x3FF,
		.id_val = 0x009,
		.note = "MDMCU: CK2 MCUP+ HW:ARM I PORT",
		.name = "MDMCU_C2K"
	},
	{
		.master = MST_ID_C2KMCU_2,
		.port = 0x3,
		.id_mask = 0x3FF,
		.id_val = 0x013,
		.note = "MDMCU: CK2 MCUP+ HW:DMA PORT",
		.name = "MDMCU_C2K"
	},

	/* Modem HW (2G/3G) */
	{
		.master = MST_ID_MDHW_0,
		.port = 0x4,
		.id_mask = 0x3FF,
		.id_val = 0x000,
		.note = "MDHW: MDGDMA",
		.name = "MDHW_MDGDMA"
	},
	{
		.master = MST_ID_MDHW_1,
		.port = 0x4,
		.id_mask = 0x3FB,
		.id_val = 0x001,
		.note = "MDHW: CLDMA_MD",
		.name = "MDHW_CLDMA_MD"
	},
	{
		.master = MST_ID_MDHW_2,
		.port = 0x4,
		.id_mask = 0x3FB,
		.id_val = 0x002,
		.note = "MDHW: ABM",
		.name = "MDHW_ABM"
	},
	{
		.master = MST_ID_MDHW_3,
		.port = 0x4,
		.id_mask = 0x3FB,
		.id_val = 0x003,
		.note = "MDHW: CLDMA_AP",
		.name = "MDHW_CLDMA_AP"
	},
	{
		.master = MST_ID_MDHW_4,
		.port = 0x4,
		.id_mask = 0x3F8,
		.id_val = 0x008,
		.note = "MDHW: MODEMSYS",
		.name = "MDHW_MODEMSYS"
	},
	{
		.master = MST_ID_MDHW_5,
		.port = 0x4,
		.id_mask = 0x3FE,
		.id_val = 0x010,
		.note = "MDHW: LTEL2_DMA",
		.name = "MDHW_LTEL2_DMA"
	},
	{
		.master = MST_ID_MDHW_6,
		.port = 0x4,
		.id_mask = 0x3FF,
		.id_val = 0x018,
		.note = "MDHW: TDDSYS, LTEL1_DMA",
		.name = "MDHW_TDDSYS_LTEL1_DMA"
	},
	{
		.master = MST_ID_MDHW_7,
		.port = 0x4,
		.id_mask = 0x3FE,
		.id_val = 0x020,
		.note = "MDHW: MODEM_HARQ",
		.name = "MDHW_MODEM_HARQ"
	},
	{
		.master = MST_ID_MDHW_8,
		.port = 0x4,
		.id_mask = 0x3FF,
		.id_val = 0x100,
		.note = "MDHW: HARQ ID",
		.name = "MDHW_HARQ_ID"
	},
	{
		.master = MST_ID_MDHW_9,
		.port = 0x4,
		.id_mask = 0x3FF,
		.id_val = 0x102,
		.note = "MDHW: POSITION ID",
		.name = "MDHW_POSITION_ID"
	},
	/* MFG  */
	{
		.master = MST_ID_MFG_0,
		.port = 0x5,
		.id_mask = 0x3C0,
		.id_val = 0x000,
		.note = "MFG",
		.name = "MFG"
	},

};

static const char *UNKNOWN_MASTER = "unknown";
static spinlock_t emi_mpu_lock;

#ifdef ENABLE_EMI_CHKER
struct timer_list emi_axi_vio_timer;
#endif

char *smi_larb0_port[7] = {
	"disp_ovl_0",
	"disp_rdma_0",
	"disp_wdma_0",
	"disp_rdma_1",
	"mdp_rdma",
	"mdp_wdma",
	"mdp_wrot"
};

char *smi_larb1_port[7] =  {
	"hw_vdec_mc_ext",
	"hw_vdec_pp_ext",
	"hw_vdec_avc_mv_ext" ,
	"hw_vdec_pred_rd_ext",
	"hw_vdec_pred_wr_ext",
	"hw_vdec_vld_ext",
	"hw_vdec_ppwarp_ext"
};

char *smi_larb2_port[21] = {
	"cam_imgo",
	"cam_rrzo",
	"cam_aao",
	"cam_isco",
	"cam_esfko",
	"cam_imgo_s",
	"cam_isci",
	"cam_isci_d",
	"cam_bpci",
	"cam_bpci_d",
	"cam_ufdi",
	"cam_imgi",
	"cam_img2o",
	"cam_img3o",
	"cam_vipi",
	"cam_vip2i",
	"cam_vip3i",
	"cam_icei" ,
	"cam_rb" ,
	"cam_rp" ,
	"cam_wr"
};

char *smi_larb3_port[13] = {
	"venc_rcpu",
	"venc_rec",
	"venc_bsdma",
	"venc_sv_comv",
	"venc_rd_comv",
	"jpgenc_rdma",
	"jpgenc_bsdma",
	"jpgdec_wdma",
	"jpgdec_bsdma",
	"venc_cur_luma",
	"venc_cur_chroma",
	"venc_ref_luma",
	"vend_ref_chroma"
};

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID)
{
	u32 mm_larb;
	u32 smi_port;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val) &&
	    (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case 0: /* ARM */
		case 2: /* Peripheral */
		case 3: /* MDMCU ,C2K MCU */
		case 4: /* MD HW ,LTE */
		case 5: /* MFG */
		case 6:
		case 7:
			pr_err("Violation master name is %s.\n",
			mst_tbl[tbl_idx].name);
			break;
		case 1: /*MM*/
			mm_larb = axi_id>>7;
			smi_port = (axi_id & 0x7F) >> 2;
			if (mm_larb == 0x0) {
				/*smi_larb0 */
				if (smi_port >= ARRAY_SIZE(smi_larb0_port)) {
					pr_err("[EMI MPU ERROR] Invalidatemaster ID! %s",
					       "lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				       mst_tbl[tbl_idx].name,
				       smi_larb0_port[smi_port]);
			} else if (mm_larb == 0x1) {
				/*smi_larb1*/
				if (smi_port >= ARRAY_SIZE(smi_larb1_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! %s",
					       "lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				       mst_tbl[tbl_idx].name,
				       smi_larb1_port[smi_port]);
			} else if (mm_larb == 0x2) {
				/*smi_larb2*/
				if (smi_port >= ARRAY_SIZE(smi_larb2_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! %s",
					       "lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				       mst_tbl[tbl_idx].name,
				       smi_larb2_port[smi_port]);
			} else if (mm_larb == 0x3) {
				/*smi_larb3*/
				if (smi_port >= ARRAY_SIZE(smi_larb3_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! %s",
					       "lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				       mst_tbl[tbl_idx].name,
				       smi_larb3_port[smi_port]);
			} else /*MM IOMMU Internal Used*/ {
				pr_err("Violation master name is %s.\n",
				       mst_tbl[tbl_idx].name);
			}
			break;
		default:
			pr_err("[EMI MPU ERROR] Invalidate port ID! lookup bus ID table failed!\n");
			break;
		}
		return 1;
	} else {
		return 0;
	}
}

static u32 __id2mst(u32 id)
{

	int i;
	u32 axi_ID;
	u32 port_ID;

	axi_ID = (id >> 3) & 0x000001FFF;
	port_ID = id & 0x00000007;

	pr_err("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))
			return mst_tbl[i].master;
	}

	return MST_INVALID;
}

static char *__id2name(u32 id)
{

	int i;
	u32 axi_ID;
	u32 port_ID;

	axi_ID = (id >> 3) & 0x00001FFF;
	port_ID = id & 0x00000007;

	pr_err("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))
			return mst_tbl[i].name;
	}

	return (char *)UNKNOWN_MASTER;
}

static void __clear_emi_mpu_vio(void)
{
	u32 dbg_s, dbg_t;

	/* clear violation status */
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUP);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUQ);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUR);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUY);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUP2);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUQ2);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUR2);
	mt_reg_sync_writel(0x00FF03FF, EMI_MPUY2);

	/* clear debug info */
	mt_reg_sync_writel(0x80000000, EMI_MPUS);
	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));

	/* MT6582 EMI hw bug EMI_MPUS[10:0] and EMI_MPUT can't be cleared */
	if (dbg_s) {
		pr_err("Fail to clear EMI MPU violation\n");
		pr_err("EMI_MPUS = %x, EMI_MPUT = %x", dbg_s, dbg_t);
	}
}

/*EMI MPU violation handler*/
static irqreturn_t mpu_violation_irq(int irq, void *dev_id)
{
	u32 dbg_s, dbg_t, dbg_pqry;
	u32 master_ID, domain_ID, wr_vio;
	s32 region;
	int res;
	char *master_name;


	/* Need DEVAPC owner porting */
	res = mt_devapc_check_emi_violation();
	if (res)
		return IRQ_NONE;

	pr_debug("It's a MPU violation.\n");

	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));

	pr_debug("Clear status.\n");

	/*in 6735 Master ID valid bit is [13:0] */
	master_ID = (dbg_s & 0x00003FFF);
	domain_ID = (dbg_s >> 21) & 0x00000007;
	wr_vio = (dbg_s >> 28) & 0x00000003;
	region = (dbg_s >> 16) & 0xF;


	switch (domain_ID) {
	case 0:
		dbg_pqry = readl(IOMEM(EMI_MPUP));
		break;
	case 1:
		dbg_pqry = readl(IOMEM(EMI_MPUQ));
		break;
	case 2:
		dbg_pqry = readl(IOMEM(EMI_MPUR));
		break;
	case 3:
		dbg_pqry = readl(IOMEM(EMI_MPUY));
		break;

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	case 4:
		dbg_pqry = readl(IOMEM(EMI_MPUP2));
		break;
	case 5:
		dbg_pqry = readl(IOMEM(EMI_MPUQ2));
		break;
	case 6:
		dbg_pqry = readl(IOMEM(EMI_MPUR2));
		break;
	case 7:
		dbg_pqry = readl(IOMEM(EMI_MPUY2));
		break;
#endif

	default:
		dbg_pqry = 0;
		break;
	}

	/*TBD: print the abort region*/

	pr_err("EMI MPU violation.\n");
	pr_err("[EMI MPU] Debug info start ----------------------------------------\n");

	pr_err("EMI_MPUS = %x, EMI_MPUT = %x.\n", dbg_s, dbg_t);
	pr_err("Current process is \"%s \" (pid: %i).\n",
	current->comm, current->pid);
	pr_err("Violation address is 0x%x.\n", dbg_t + emi_physical_offset);
	pr_err("Violation master ID is 0x%x.\n", master_ID);
	/*print out the murderer name*/
	master_name = __id2name(master_ID);
	pr_err("Violation domain ID is 0x%x.\n", domain_ID);
	pr_err("%s violation.\n", (wr_vio == 1) ? "Write" : "Read");
	pr_err("Corrupted region is %d\n\r", region);
	if (dbg_pqry & OOR_VIO)
		pr_err("Out of range violation.\n");

	pr_err("[EMI MPU] Debug info end------------------------------------------\n");

#if 0
	/* For MDHW debug usage, 0x6C -> MDHW, Master is 3G */
	if (dbg_s & 0x6C)
		exec_ccci_kern_func_by_md_id(0, ID_FORCE_MD_ASSERT, NULL, 0);
#endif
#ifdef CONFIG_MTK_AEE_FEATURE
	/*FIXME: skip ca53 violation to trigger root-cause KE*/
	if ((0 != dbg_s) &&
	    (__id2mst(master_ID) != MST_ID_APMCU_0)
	    && (__id2mst(master_ID) != MST_ID_APMCU_1)) {
		aee_kernel_exception("EMI MPU",
		"EMI MPU violation.\nEMI_MPUS = 0x%x,EMI_MPUT = 0x%x\nCRDISPATCH_KEY:EMI MPU Violation Issue/%s\n",
		dbg_s, dbg_t+emi_physical_offset, master_name);
	}
#endif

	__clear_emi_mpu_vio();

	mt_devapc_clear_emi_violation();

#if 0
	list_for_each(p, &(emi_mpu_notifier_list[__id2mst(master_ID)])) {
		block = list_entry(p, struct emi_mpu_notifier_block, list);
		block->notifier(dbg_t + emi_physical_offset, wr_vio);
	}
#endif

	vio_addr = dbg_t + emi_physical_offset;

	return IRQ_HANDLED;
}

#if defined(CONFIG_ARCH_MT6753)
/* Acquire DRAM Setting for PASR/DPD */
void acquire_dram_setting(struct basic_dram_setting *pasrdpd)
{
	int ch_nr = MAX_CHANNELS;
	unsigned int emi_cona, emi_conh, col_bit, row_bit;
	unsigned int ch0_rank0_size, ch0_rank1_size;
	unsigned int ch1_rank0_size, ch1_rank1_size;

	pasrdpd->channel_nr = ch_nr;

	emi_cona = readl(IOMEM(EMI_CONA));
	emi_conh = readl(IOMEM(EMI_CONH));

	ch0_rank0_size = (emi_conh >> 16) & 0xf;
	ch0_rank1_size = (emi_conh >> 20) & 0xf;
	ch1_rank0_size = (emi_conh >> 24) & 0xf;
	ch1_rank1_size = (emi_conh >> 28) & 0xf;

	/*channel 0*/
	{
		/*rank 0*/
		pasrdpd->channel[0].rank[0].valid_rank = true;

		if (ch0_rank0_size == 0) {
			col_bit = ((emi_cona >> 4) & 0x03) + 9;
			row_bit = ((emi_cona >> 12) & 0x03) + 13;
			/* 32 bits * 8 banks, unit Gb*/
			pasrdpd->channel[0].rank[0].rank_size =
			(1 << (row_bit + col_bit)) >> 22;
			pasrdpd->channel[0].rank[0].segment_nr = 8;
		} else {
			pasrdpd->channel[0].rank[0].rank_size =
			(ch0_rank0_size * 2);
			/*unit Gb*/
			pasrdpd->channel[0].rank[0].segment_nr = 6;
		}

		if (0 != (emi_cona & (1 << 17))) {
			/*rank 1 exist*/
			pasrdpd->channel[0].rank[1].valid_rank = true;

			if (ch0_rank1_size == 0) {
				col_bit = ((emi_cona >> 6) & 0x03) + 9;
				row_bit = ((emi_cona >> 14) & 0x03) + 13;
				pasrdpd->channel[0].rank[1].rank_size =
				(1 << (row_bit + col_bit)) >> 22;
				/*32 bits * 8 banks, unit Gb*/
				pasrdpd->channel[0].rank[1].segment_nr = 8;
			} else {
				pasrdpd->channel[0].rank[1].rank_size =
				(ch0_rank1_size * 2);
				/* unit Gb*/
				pasrdpd->channel[0].rank[1].segment_nr = 6;
			}
		} else {
			pasrdpd->channel[0].rank[1].valid_rank = false;
			pasrdpd->channel[0].rank[1].segment_nr = 0;
			pasrdpd->channel[0].rank[1].rank_size = 0;
		}
	}

	if (0 != (emi_cona & 0x01)) {
		/*channel 1 exist*/
		/*rank0 setting*/
		pasrdpd->channel[1].rank[0].valid_rank = true;

		if (ch1_rank0_size == 0) {
			col_bit = ((emi_cona >> 20) & 0x03) + 9;
			row_bit = ((emi_cona >> 28) & 0x03) + 13;
			pasrdpd->channel[1].rank[0].rank_size =
			(1 << (row_bit + col_bit)) >> 22;
			/* 32 bits * 8 banks, unit Gb*/
			pasrdpd->channel[1].rank[0].segment_nr = 8;
		} else {
			pasrdpd->channel[1].rank[0].rank_size =
			(ch1_rank0_size * 2);
			/* unit Gb*/
			pasrdpd->channel[1].rank[0].segment_nr = 6;
		}

		if (0 != (emi_cona & (1 << 16))) {
			/*rank 1 exist*/

			pasrdpd->channel[1].rank[1].valid_rank = true;

			if (ch1_rank1_size == 0) {
				col_bit = ((emi_cona >> 22) & 0x03) + 9;
				row_bit = ((emi_cona >> 30) & 0x03) + 13;
				pasrdpd->channel[1].rank[1].rank_size =
				(1 << (row_bit + col_bit)) >> 22;
				/* 32 bits * 8 banks, unit Gb*/
				pasrdpd->channel[1].rank[1].segment_nr = 8;
			} else {
				pasrdpd->channel[1].rank[1].rank_size =
				(ch1_rank1_size * 2);
				/* unit Gb*/
				pasrdpd->channel[1].rank[1].segment_nr = 6;
			}
		} else {
			pasrdpd->channel[1].rank[1].valid_rank = false;
			pasrdpd->channel[1].rank[1].segment_nr = 0;
			pasrdpd->channel[1].rank[1].rank_size = 0;
		}
	} else {
		/*channel 2 does not exist*/
		pasrdpd->channel[1].rank[0].valid_rank = false;
		pasrdpd->channel[1].rank[0].segment_nr = 0;
		pasrdpd->channel[1].rank[0].rank_size = 0;

		pasrdpd->channel[1].rank[1].valid_rank = false;
		pasrdpd->channel[1].rank[1].segment_nr = 0;
		pasrdpd->channel[1].rank[1].rank_size = 0;
	}
}
#endif

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned int start,
unsigned int end, int region, unsigned int access_permission)
{
	int ret = 0;
	unsigned int tmp, tmp2;
	unsigned int ax_pm, ax_pm2;
	unsigned long flags;

	if ((end != 0) || (start != 0)) {
		/*Address 64KB alignment*/
		start -= emi_physical_offset;
		end -= emi_physical_offset;
		start = start >> 16;
		end = end >> 16;

		if (end < start)
			return -EINVAL;
	}

	ax_pm = (access_permission << 16) >> 16;
	ax_pm2 = (access_permission >> 16);

	spin_lock_irqsave(&emi_mpu_lock, flags);

	switch (region) {
	case 0:
		/* Clear access right before setting MPU address*/
		tmp = readl(IOMEM(EMI_MPUI)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUI_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUI);
		mt_reg_sync_writel(0, EMI_MPUI_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUA);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUI);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUI_2ND);
		break;

	case 1:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUI)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUI_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUI);
		mt_reg_sync_writel(0, EMI_MPUI_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUB);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUI);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUI_2ND);
		break;

	case 2:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUJ)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUJ_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUJ);
		mt_reg_sync_writel(0, EMI_MPUJ_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUC);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUJ);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUJ_2ND);
		break;

	case 3:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUJ)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUJ_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUJ);
		mt_reg_sync_writel(0, EMI_MPUJ_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUD);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUJ);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUJ_2ND);
		break;

	case 4:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUK)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUK_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUK);
		mt_reg_sync_writel(0, EMI_MPUK_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUE);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUK);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUK_2ND);
		break;

	case 5:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUK)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUK_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUK);
		mt_reg_sync_writel(0, EMI_MPUK_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUF);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUK);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUK_2ND);
		break;

	case 6:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUL)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUL_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUL);
		mt_reg_sync_writel(0, EMI_MPUL_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUG);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUL);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUL_2ND);
		break;

	case 7:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUL)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUL_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUL);
		mt_reg_sync_writel(0, EMI_MPUL_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUH);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUL);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUL_2ND);
		break;

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	case 8:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUI2)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUI2_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUI2);
		mt_reg_sync_writel(0, EMI_MPUI2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUA2);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUI2);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUI2_2ND);
		break;

	case 9:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUI2)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUI2_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUI2);
		mt_reg_sync_writel(0, EMI_MPUI2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUB2);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUI2);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUI2_2ND);
		break;

	case 10:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUJ2)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUJ2_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUJ2);
		mt_reg_sync_writel(0, EMI_MPUJ2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUC2);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUJ2);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUJ2_2ND);
		break;

	case 11:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUJ2)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUJ2_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUJ2);
		mt_reg_sync_writel(0, EMI_MPUJ2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUD2);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUJ2);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUJ2_2ND);
		break;

	case 12:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUK2)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUK2_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUK2);
		mt_reg_sync_writel(0, EMI_MPUK2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUE2);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUK2);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUK2_2ND);
		break;

	case 13:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUK2)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUK2_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUK2);
		mt_reg_sync_writel(0, EMI_MPUK2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUF2);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUK2);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUK2_2ND);
		break;

	case 14:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUL2)) & 0xFFFF0000;
		tmp2 = readl(IOMEM(EMI_MPUL2_2ND)) & 0xFFFF0000;
		mt_reg_sync_writel(0, EMI_MPUL2);
		mt_reg_sync_writel(0, EMI_MPUL2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUG2);
		mt_reg_sync_writel(tmp | ax_pm, EMI_MPUL2);
		mt_reg_sync_writel(tmp2 | ax_pm2, EMI_MPUL2_2ND);
		break;

	case 15:
		/* Clear access right before setting MPU address */
		tmp = readl(IOMEM(EMI_MPUL2)) & 0x0000FFFF;
		tmp2 = readl(IOMEM(EMI_MPUL2_2ND)) & 0x0000FFFF;
		mt_reg_sync_writel(0, EMI_MPUL2);
		mt_reg_sync_writel(0, EMI_MPUL2_2ND);
		mt_reg_sync_writel((start << 16) | end, EMI_MPUH2);
		mt_reg_sync_writel(tmp | (ax_pm << 16), EMI_MPUL2);
		mt_reg_sync_writel(tmp2 | (ax_pm2 << 16), EMI_MPUL2_2ND);
		break;
#endif

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock_irqrestore(&emi_mpu_lock, flags);

#if 0
	pr_debug("[EMI MPU] emi_physical_offset = 0x%x\n", emi_physical_offset);
	pr_debug("[EMI MPU] EMI_MPUA = 0x%x\n", readl(IOMEM(EMI_MPUA)));
	pr_debug("[EMI MPU] EMI_MPUB = 0x%x\n", readl(IOMEM(EMI_MPUB)));
	pr_debug("[EMI MPU] EMI_MPUC = 0x%x\n", readl(IOMEM(EMI_MPUC)));
	pr_debug("[EMI MPU] EMI_MPUD = 0x%x\n", readl(IOMEM(EMI_MPUD)));
	pr_debug("[EMI MPU] EMI_MPUE = 0x%x\n", readl(IOMEM(EMI_MPUE)));
	pr_debug("[EMI MPU] EMI_MPUF = 0x%x\n", readl(IOMEM(EMI_MPUF)));
	pr_debug("[EMI MPU] EMI_MPUG = 0x%x\n", readl(IOMEM(EMI_MPUG)));
	pr_debug("[EMI MPU] EMI_MPUH = 0x%x\n", readl(IOMEM(EMI_MPUH)));
	pr_debug("[EMI MPU] EMI_MPUA2 = 0x%x\n", readl(IOMEM(EMI_MPUA2)));
	pr_debug("[EMI MPU] EMI_MPUB2 = 0x%x\n", readl(IOMEM(EMI_MPUB2)));
	pr_debug("[EMI MPU] EMI_MPUC2 = 0x%x\n", readl(IOMEM(EMI_MPUC2)));
	pr_debug("[EMI MPU] EMI_MPUD2 = 0x%x\n", readl(IOMEM(EMI_MPUD2)));
	pr_debug("[EMI MPU] EMI_MPUE2 = 0x%x\n", readl(IOMEM(EMI_MPUE2)));
	pr_debug("[EMI MPU] EMI_MPUF2 = 0x%x\n", readl(IOMEM(EMI_MPUF2)));
	pr_debug("[EMI MPU] EMI_MPUG2 = 0x%x\n", readl(IOMEM(EMI_MPUG2)));
	pr_debug("[EMI MPU] EMI_MPUH2 = 0x%x\n", readl(IOMEM(EMI_MPUH2)));
#endif

	return ret;
}
EXPORT_SYMBOL(emi_mpu_set_region_protection);

/*
 * emi_mpu_notifier_register: register a notifier.
 * master: MST_ID_xxx
 * notifier: the callback function
 * Return 0 for success, otherwise negative error code.
 */
#if 0
int emi_mpu_notifier_register(int master, emi_mpu_notifier notifier)
{
	struct emi_mpu_notifier_block *block;
	static int emi_mpu_notifier_init;
	int i;

	if (master >= MST_INVALID)
		return -EINVAL;

	block = kmalloc(sizeof(struct emi_mpu_notifier_block), GFP_KERNEL);
	if (!block)
		return -ENOMEM;

	if (!emi_mpu_notifier_init) {
		for (i = 0; i < NR_MST; i++)
			INIT_LIST_HEAD(&(emi_mpu_notifier_list[i]));

		emi_mpu_notifier_init = 1;
	}

	block->notifier = notifier;
	list_add(&(block->list), &(emi_mpu_notifier_list[master]));

	return 0;
}
#endif

static ssize_t emi_mpu_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	unsigned int start, end;
	unsigned int reg_value;
	unsigned int d0, d1, d2, d3;
#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	unsigned int d4, d5, d6, d7, reg_value2;
#endif

	static const char *permission[7] = {
		"No protect",
		"Only R/W for secure access",
		"Only R/W for secure access, and non-secure read access",
		"Only R/W for secure access, and non-secure write access",
		"Only R for secure/non-secure",
		"Both R/W are forbidden",
		"Only secure W is forbidden"
	};

	reg_value = readl(IOMEM(EMI_MPUA));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 0 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUB));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 1 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUC));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 2 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUD));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 3 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUE));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 4 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUF));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 5 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUG));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 6 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUH));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 7 --> 0x%x to 0x%x\n", start, end);

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	reg_value = readl(IOMEM(EMI_MPUA2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 8 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUB2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 9 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUC2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 10 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUD2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 11 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUE2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 12 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUF2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 13 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUG2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 14 --> 0x%x to 0x%x\n", start, end);

	reg_value = readl(IOMEM(EMI_MPUH2));
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "Region 15 --> 0x%x to 0x%x\n", start, end);
#endif

	ptr += sprintf(ptr, "\n");

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	reg_value = readl(IOMEM(EMI_MPUI));
	reg_value2 = readl(IOMEM(EMI_MPUI_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 0 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 0 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 1 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 1 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	reg_value = readl(IOMEM(EMI_MPUJ));
	reg_value2 = readl(IOMEM(EMI_MPUJ_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 2 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 2 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 3 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 3 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);


	reg_value = readl(IOMEM(EMI_MPUK));
	reg_value2 = readl(IOMEM(EMI_MPUK_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 4 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 4 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);


	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 5 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 5 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	reg_value = readl(IOMEM(EMI_MPUL));
	reg_value2 = readl(IOMEM(EMI_MPUL_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 6 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 6 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 7 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 7 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	reg_value = readl(IOMEM(EMI_MPUI2));
	reg_value2 = readl(IOMEM(EMI_MPUI2_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 8 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 8 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 9 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr, "Region 9 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	reg_value = readl(IOMEM(EMI_MPUJ2));
	reg_value2 = readl(IOMEM(EMI_MPUJ2_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr,
	"Region 10 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr,
	"Region 10 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr,
	"Region 11 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr,
	"Region 11 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);


	reg_value = readl(IOMEM(EMI_MPUK2));
	reg_value2 = readl(IOMEM(EMI_MPUK2_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr,
	"Region 12 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr,
	"Region 12 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);


	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr,
	"Region 13 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr,
	"Region 13 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	reg_value = readl(IOMEM(EMI_MPUL2));
	reg_value2 = readl(IOMEM(EMI_MPUL2_2ND));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value2 & 0x7);
	d5 = (reg_value2 >> 3) & 0x7;
	d6 = (reg_value2 >> 6) & 0x7;
	d7 = (reg_value2 >> 9) & 0x7;
	ptr += sprintf(ptr,
	"Region 14 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
	ptr += sprintf(ptr,
	"Region 14 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	d4 = ((reg_value2>>16) & 0x7);
	d5 = ((reg_value2>>16) >> 3) & 0x7;
	d6 = ((reg_value2>>16) >> 6) & 0x7;
	d7 = ((reg_value2>>16) >> 9) & 0x7;
	ptr += sprintf(ptr,
	"Region 15 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
		       ptr += sprintf(ptr,
		       "Region 15 --> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
		       permission[d4],
		       permission[d5],
		       permission[d6],
		       permission[d7]);
#else /*MT6735M*/
	reg_value = readl(IOMEM(EMI_MPUI));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 0 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 1 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	reg_value = readl(IOMEM(EMI_MPUJ));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 2 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 3 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	reg_value = readl(IOMEM(EMI_MPUK));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 4 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 5 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	reg_value = readl(IOMEM(EMI_MPUL));
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 6 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);

	d0 = ((reg_value>>16) & 0x7);
	d1 = ((reg_value>>16) >> 3) & 0x7;
	d2 = ((reg_value>>16) >> 6) & 0x7;
	d3 = ((reg_value>>16) >> 9) & 0x7;
	ptr += sprintf(ptr, "Region 7 --> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
		       permission[d0],
		       permission[d1],
		       permission[d2],
		       permission[d3]);
#endif
	return strlen(buf);
}

static ssize_t emi_mpu_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int i;
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned int region;
	unsigned int access_permission;
	char *command;
	char *ptr;
	char *token[5];
	int err = 0;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_mpu_store command overflow.");
		return count;
	}
	pr_err("emi_mpu_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strncpy(command, buf, (size_t) MAX_EMI_MPU_STORE_CMD_LEN);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_MPU_STR, strlen(EN_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_debug("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 5; i++)
			pr_debug("token[%d] = %s\n", i, token[i]);

		err += kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		err += kstrtoul(token[3], 16, (unsigned long *)&region);
		err += kstrtoul(token[4],
		16, (unsigned long *)&access_permission);

		if (err)
			goto out;


		emi_mpu_set_region_protection(start_addr,
					      end_addr,
					      region,
					      access_permission);
		pr_err("Set EMI_MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x.\n",
		       start_addr,
		       end_addr,
		       region,
		       access_permission);
	} else if (!strncmp(buf, DIS_MPU_STR, strlen(DIS_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_debug("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 5; i++)
			pr_debug("token[%d] = %s\n", i, token[i]);

		err += kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		err += kstrtoul(token[3], 16, (unsigned long *)&region);

		if (err)
			goto out;

		access_permission = SET_ACCESS_PERMISSON(NO_PROTECTION,
#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
							 NO_PROTECTION,
							 NO_PROTECTION,
							 NO_PROTECTION,
							 NO_PROTECTION,
#endif
							 NO_PROTECTION,
							 NO_PROTECTION,
							 NO_PROTECTION);
		emi_mpu_set_region_protection(0x0,
					      0x0,
					      region,
					      access_permission);
		pr_err("set EMI MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x\n",
		       0,
		       0,
		       region,
		       access_permission);
	} else {
		pr_err("Unknown emi_mpu command.\n");
	}

out:
	kfree(command);

	return count;
}

DRIVER_ATTR(mpu_config, 0644, emi_mpu_show, emi_mpu_store);

void mtk_search_full_pgtab(void)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr;
#ifndef CONFIG_ARM_LPAE
	pte_t *pte;
	unsigned long addr_2nd, addr_2nd_end;
#endif
	unsigned int v_addr = vio_addr;

	/*FIXME: testing*/

	for (addr = 0xC0000000; addr < 0xFFF00000; addr += 0x100000) {
		pgd = pgd_offset(&init_mm, addr);
		if (pgd_none(*pgd) || !pgd_present(*pgd))
			continue;

		pud = pud_offset(pgd, addr);
		if (pud_none(*pud) || !pud_present(*pud))
			continue;

		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || !pmd_present(*pmd))
			continue;

#ifndef CONFIG_ARM_LPAE
		if ((pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE) {
			/* Page table entry*/
			addr_2nd = addr;
			addr_2nd_end = addr_2nd + 0x100000;
			for (; addr_2nd < (addr_2nd_end); addr_2nd += 0x1000) {
				pte = pte_offset_map(pmd, addr_2nd);
				if (((unsigned long)v_addr & PAGE_MASK) ==
					((unsigned long)pte_val(*(pte))
					& PAGE_MASK)) {
					pr_err("%s %lx. violation address = 0x%x\n",
					       "[EMI MPU] Find page entry section at pte:",
					       (unsigned long)(pte), v_addr);
					return;
				}
			}
		} else {
			/* Section */
			unsigned long a0;
			unsigned long a1;

			a0 = (unsigned long)pmd_val(*(pmd)) & SECTION_MASK;
			a1 = (unsigned long)v_addr & SECTION_MASK;

			if (a0 == a1) {
				pr_err("[EMI MPU] Find page entry section at pmd: %lx. violation address = 0x%x\n",
				       (unsigned long)(pmd),
				       v_addr);
				return;
			}
		}
#else
		/* TBD */
#endif
	}
	pr_err("[EMI MPU] ****** Can not find page table entry! violation address = 0x%x ******\n",
	v_addr);
}

void emi_mpu_work_callback(struct work_struct *work)
{
	pr_err("[EMI MPU] Enter EMI MPU workqueue!\n");
	mtk_search_full_pgtab();
	pr_err("[EMI MPU] Exit EMI MPU workqueue!\n");
}

static ssize_t pgt_scan_show(struct device_driver *driver, char *buf)
{
	return 0;
}

static ssize_t pgt_scan_store(struct device_driver *driver,
const char *buf, size_t count)
{
	unsigned int value;
	unsigned int ret;

	if (unlikely(kstrtoint(buf, 0, &value) != 1))
		return -EINVAL;

	if (value == 1)	{
		ret = queue_work(emi_mpu_workqueue, &emi_mpu_work);
		if (!ret)
			pr_debug("[EMI MPU] submit workqueue failed, ret = %d\n",
			ret);
	}

	return count;
}
DRIVER_ATTR(pgt_scan, 0644, pgt_scan_show, pgt_scan_store);

#ifdef ENABLE_EMI_CHKER
static void emi_axi_set_chker(const unsigned int setting)
{
	int value;

	value = readl(IOMEM(EMI_CHKER));
	value &= ~(0x7 << 16);
	value |= (setting);

	mt_reg_sync_writel(value, EMI_CHKER);
}

static void emi_axi_set_master(const unsigned int setting)
{
	int value;

	value = readl(IOMEM(EMI_CHKER));
	value &= ~(0x0F << AXI_NON_ALIGN_CHK_MST);
	value |= (setting & 0xF) << AXI_NON_ALIGN_CHK_MST;

	mt_reg_sync_writel(value, EMI_CHKER);
}

static void emi_axi_dump_info(int aee_ke_en)
{
	int value, master_ID;
	char *master_name;

	value = readl(IOMEM(EMI_CHKER));
	master_ID = (value & 0x0000FFFF);

	if (value & 0x0000FFFF)	{
		pr_err("AXI violation.\n");
		pr_err("[EMI MPU AXI] Debug info start ------\n");

		pr_err("EMI_CHKER = %x.\n", value);
		pr_err("Violation address is 0x%x.\n",
		readl(IOMEM(EMI_CHKER_ADR)));
		pr_err("Violation master ID is 0x%x.\n", master_ID);
		pr_err("Violation type is: AXI_ADR_CHK_EN(%d), AXI_LOCK_CHK_EN(%d), AXI_NON_ALIGN_CHK_EN(%d).\n",
		       (value & (1 << AXI_ADR_VIO)) ? 1 : 0,
		       (value & (1 << AXI_LOCK_ISSUE)) ? 1 : 0,
		       (value & (1 << AXI_NON_ALIGN_ISSUE)) ? 1 : 0);
		pr_err("%s violation.\n",
		       (value & (1 << AXI_VIO_WR)) ? "Write" : "Read");

		pr_err("[EMI MPU AXI] Debug info end ---------\n");

		master_name = __id2name(master_ID);
#ifdef CONFIG_MTK_AEE_FEATURE
		if (aee_ke_en)
			aee_kernel_exception("EMI MPU AXI",
			"AXI violation.\nEMI_CHKER = 0x%x\nCRDISPATCH_KEY:EMI MPU Violation Issue/%s\n",
			value, master_name);
#endif
		mt_reg_sync_writel((1 << AXI_VIO_CLR) | readl(IOMEM(EMI_CHKER)),
		EMI_CHKER);
	}
}

static void emi_axi_vio_timer_func(unsigned long a)
{
	emi_axi_dump_info(1);

	mod_timer(&emi_axi_vio_timer, jiffies + AXI_VIO_MONITOR_TIME);
}

static ssize_t emi_axi_vio_show(struct device_driver *driver, char *buf)
{
	int value;

	value = readl(IOMEM(EMI_CHKER));

	emi_axi_dump_info(0);

	return snprintf(buf,
			PAGE_SIZE,
			"AXI vio setting is: ADR_CHK_EN %s, LOCK_CHK_EN %s, NON_ALIGN_CHK_EN %s\n",
			(value & (1 << AXI_ADR_CHK_EN)) ? "ON" : "OFF",
			(value & (1 << AXI_LOCK_CHK_EN)) ? "ON" : "OFF",
			(value & (1 << AXI_NON_ALIGN_CHK_EN)) ? "ON" : "OFF");
}

ssize_t emi_axi_vio_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int value;
	int cpu = 0;

	value = readl(IOMEM(EMI_CHKER));

	if (!strncmp(buf, "ADR_CHK_ON", strlen("ADR_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_ADR_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "LOCK_CHK_ON", strlen("LOCK_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_LOCK_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "NON_ALIGN_CHK_ON",
		strlen("NON_ALIGN_CHK_ON"))) {
		emi_axi_set_chker(1 << AXI_NON_ALIGN_CHK_EN);
		add_timer_on(&emi_axi_vio_timer, cpu);
	} else if (!strncmp(buf, "OFF", strlen("OFF"))) {
		emi_axi_set_chker(0);
		del_timer(&emi_axi_vio_timer);
	} else {
		pr_err("invalid setting\n");
	}

	return count;
}

DRIVER_ATTR(emi_axi_vio,    0644, emi_axi_vio_show,     emi_axi_vio_store);

#endif /*#ifdef ENABLE_EMI_CHKER*/

#ifdef ENABLE_EMI_WATCH_POINT
static void emi_wp_set_address(unsigned int address)
{
	mt_reg_sync_writel(address - emi_physical_offset, EMI_WP_ADR);
}

static void emi_wp_set_range(unsigned int range)	/* 2^ range bytes */
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value = (value & (~EMI_WP_RANGE)) | range;
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

static void emi_wp_set_monitor_type(unsigned int type)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & (~EMI_WP_RW_MONITOR)) | (type << EMI_WP_RW_MONITOR_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

#if 0
static void emi_wp_set_rw_disable(unsigned int type)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & (~EMI_WP_RW_DISABLE)) | (type << EMI_WP_RW_DISABLE_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}
#endif

static void emi_wp_enable(int enable)
{
	unsigned int value;

	/* Enable WP */
	value = readl(IOMEM(EMI_CHKER));
	value =
	(value & ~(1 << EMI_WP_ENABLE_SHIFT)) | (enable << EMI_WP_ENABLE_SHIFT);
	mt_reg_sync_writel(value, EMI_CHKER);
}

static void emi_wp_slave_error_enable(unsigned int enable)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & ~(1 << EMI_WP_SLVERR_SHIFT)) | (enable << EMI_WP_SLVERR_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

static void emi_wp_int_enable(unsigned int enable)
{
	unsigned int value;

	value = readl(IOMEM(EMI_WP_CTRL));
	value =
	(value & ~(1 << EMI_WP_INT_SHIFT)) | (enable << EMI_WP_INT_SHIFT);
	mt_reg_sync_writel(value, EMI_WP_CTRL);
}

static void emi_wp_clr_status(void)
{
	unsigned int value;
	int result;

	value = readl(IOMEM(EMI_CHKER));
	value |= 1 << EMI_WP_VIO_CLR_SHIFT;
	mt_reg_sync_writel(value, EMI_CHKER);

	result = readl(IOMEM(EMI_CHKER)) & EMI_WP_AXI_ID;
	result |= readl(IOMEM(EMI_CHKER_TYPE));
	result |= readl(IOMEM(EMI_CHKER_ADR));

	if (result)
		pr_err("[EMI_WP] Clear WP status fail!!!!!!!!!!!!!!\n");
}

void emi_wp_get_status(void)
{
	unsigned int value, master_ID;
	char *master_name;

	value = readl(IOMEM(EMI_CHKER));

	if ((value & 0x80000000) == 0) {
		pr_err("[EMI_WP] No watch point hit\n");
		return;
	}

	master_ID = (value & EMI_WP_AXI_ID);
	pr_err("[EMI_WP] Violation master ID is 0x%x.\n", master_ID);
	pr_err("[EMI_WP] Violation Address is : 0x%X\n",
	       readl(IOMEM(EMI_CHKER_ADR)) + emi_physical_offset);

	master_name = __id2name(master_ID);
	pr_err("[EMI_WP] EMI_CHKER = 0x%x, module is %s.\n",
	value, master_name);

	value = readl(IOMEM(EMI_CHKER_TYPE));
	pr_err("[EMI_WP] Transaction Type is : %d beat, %d byte, %s burst type (0x%X)\n",
	       (value & 0xF) + 1,
	       1 << ((value >> 4) & 0x7),
	       (value >> 7 & 1) ? "INCR" : "WRAP",
	       value);

	emi_wp_clr_status();
}

static int emi_wp_set(unsigned int enable,
unsigned int address, unsigned int range, unsigned int rw)
{
	if (address < emi_physical_offset) {
		pr_err("[EMI_WP] Address error, you can't set address less than 0x%X\n",
		       emi_physical_offset);
		return -1;
	}
	if (range < 4 || range > 32) {
		pr_err("[EMI_WP] Range error, you can't set range less %s",
		       "than 16 bytes and more than 4G bytes\n");
		return -1;
	}

	emi_wp_set_monitor_type(rw);
	emi_wp_set_address(address);
	emi_wp_set_range(range);
	emi_wp_slave_error_enable(1);
	emi_wp_int_enable(0);
	emi_wp_enable(enable);

	return 0;
}

static ssize_t emi_wp_vio_show(struct device_driver *driver, char *buf)
{
	unsigned int value, master_ID, type, vio_addr;
	char *master_name;
	char *ptr = buf;

	value = readl(IOMEM(EMI_CHKER));

	if ((value & 0x80000000) == 0)
		return snprintf(buf, PAGE_SIZE,
		"[EMI_WP] No watch point hit\n");

	master_ID = (value & EMI_WP_AXI_ID);
	master_name = __id2name(master_ID);

	type = readl(IOMEM(EMI_CHKER_TYPE));
	vio_addr = readl(IOMEM(EMI_CHKER_ADR)) + emi_physical_offset;
	emi_wp_clr_status();
	ptr += snprintf(ptr, PAGE_SIZE, "[EMI WP] vio setting is: CHKER 0x%X, ",
			value);

	ptr += snprintf(ptr, PAGE_SIZE, "module is %s, Address is : 0x%X," ,
			master_name,
			vio_addr);

	ptr += snprintf(ptr, PAGE_SIZE, "Transaction Type is : %d beat, ",
			(type & 0xF) + 1);

	ptr += snprintf(ptr, PAGE_SIZE, "%d byte, %s burst type (0x%X)\n",
			1 << ((type >> 4) & 0x7),
			(type >> 7 & 1) ? "INCR" : "WRAP",
			type);
	return ptr - buf;
}

ssize_t emi_wp_vio_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int i;
	unsigned int wp_addr;
	unsigned int range, start_addr, end_addr;
	unsigned int rw;
	char *command;
	char *ptr;
	char *token[5];
	int err = 0;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_wp_store command overflow.");
		return count;
	}
	pr_err("emi_wp_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strncpy(command, buf, (size_t)MAX_EMI_MPU_STORE_CMD_LEN);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_WP_STR, strlen(EN_WP_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_debug("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 4; i++)
			pr_debug("token[%d] = %s\n", i, token[i]);

		err += kstrtoul(token[1], 16, (unsigned long *)&wp_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&range);
		err += kstrtoul(token[3], 16, (unsigned long *)&rw);
		if (err)
			goto out;

		emi_wp_set(1, wp_addr, range, rw);

		start_addr = (wp_addr >> range) << range;
		end_addr = start_addr + (1 << range)     - 1;
		pr_err("Set EMI_WP: address: 0x%x, range:%d, start addr: 0x%x, end addr: 0x%x,  rw: %d .\n",
		       wp_addr, range, start_addr, end_addr, rw);
	} else if (!strncmp(buf, DIS_WP_STR, strlen(DIS_WP_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_debug("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 4; i++)
			pr_debug("token[%d] = %s\n", i, token[i]);


		err += kstrtoul(token[1], 16, (unsigned long *)&wp_addr);
		err += kstrtoul(token[2], 16, (unsigned long *)&range);
		err += kstrtoul(token[3], 16, (unsigned long *)&rw);
		if (err)
			goto out;

		emi_wp_set(0, 0x40000000, 4, 2);
		pr_err("disable EMI WP\n");
	} else {
		pr_err("Unknown emi_wp command.\n");
	}

out:
	kfree(command);

	return count;
}


DRIVER_ATTR(emi_wp_vio, 0644, emi_wp_vio_show, emi_wp_vio_store);
#endif /*#ifdef ENABLE_EMI_WATCH_POINT*/

#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
#define AP_REGION_ID   15
#else /* MT6735M*/
#define AP_REGION_ID   7
#endif
static void protect_ap_region(void)
{

	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	unsigned int kernel_base;
	phys_addr_t dram_size;

	return; /* temp to disable */

	kernel_base = PHYS_OFFSET;
	dram_size = get_max_DRAM_size();

	ap_mem_mpu_id = AP_REGION_ID;
#if defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6753)
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN,
					       NO_PROTECTION,
					       FORBIDDEN,
					       NO_PROTECTION,
					       FORBIDDEN,
					       FORBIDDEN,
					       FORBIDDEN,
					       NO_PROTECTION);
#else /* MT6735M*/
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(NO_PROTECTION,
					       FORBIDDEN,
					       FORBIDDEN,
					       NO_PROTECTION);
#endif

	pr_err("[EMI] protect_ap_region 0x%x~0x%x   ,dram size=0x%x\n",
	       kernel_base,
	       (kernel_base + (unsigned int)dram_size - 1),
	       (unsigned int)dram_size);
	emi_mpu_set_region_protection(kernel_base,
	(kernel_base+dram_size-1), ap_mem_mpu_id, ap_mem_mpu_attr);
}

/*
  static int emi_mpu_panic_cb(struct notifier_block *this,
  unsigned long event, void *ptr)
  {
  emi_axi_dump_info(1);

  return NOTIFY_DONE;
  }*/

static struct platform_driver emi_mpu_ctrl = {
	.driver = {
		.name = "emi_mpu_ctrl",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
	.id_table = NULL,
};
/*
  static struct notifier_block emi_mpu_blk = {
  .notifier_call    = emi_mpu_panic_cb,
  };*/


static int __init emi_mpu_mod_init(void)
{
	int ret;
#if defined(CONFIG_ARCH_MT6753)
	struct basic_dram_setting DRAM_setting;
#endif
	struct device_node *node;
	unsigned int mpu_irq;

	pr_err("[EMI MPU] Initialize EMI MPU.\n");

	/* DTS version */

	if (EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,EMI");
		if (node) {
			EMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get EMI_BASE_ADDR @ %p\n", EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,DEVAPC");
	if (node) {
		mpu_irq = irq_of_parse_and_map(node, 0);
		pr_debug("get EMI_MPU irq = %d\n", mpu_irq);
	} else {
		pr_err("can't find compatible node DEVAPC\n");
		return -1;
	}
	spin_lock_init(&emi_mpu_lock);

	pr_err("[EMI MPU] EMI_MPUP = 0x%x\n", readl(IOMEM((EMI_MPUP))));
	pr_err("[EMI MPU] EMI_MPUQ = 0x%x\n", readl(IOMEM((EMI_MPUQ))));
	pr_err("[EMI MPU] EMI_MPUR = 0x%x\n", readl(IOMEM((EMI_MPUR))));
	pr_err("[EMI MPU] EMI_MPUY = 0x%x\n", readl(IOMEM(EMI_MPUY)));
	pr_err("[EMI MPU] EMI_MPUP2 = 0x%x\n", readl(IOMEM(EMI_MPUP2)));
	pr_err("[EMI MPU] EMI_MPUQ2 = 0x%x\n", readl(IOMEM(EMI_MPUQ2)));
	pr_err("[EMI MPU] EMI_MPUR2 = 0x%x\n", readl(IOMEM(EMI_MPUR2)));
	pr_err("[EMI MPU] EMI_MPUY2 = 0x%x\n", readl(IOMEM(EMI_MPUY2)));

	pr_err("[EMI MPU] EMI_MPUS = 0x%x\n", readl(IOMEM(EMI_MPUS)));
	pr_err("[EMI MPU] EMI_MPUT = 0x%x\n", readl(IOMEM(EMI_MPUT)));

	pr_err("[EMI MPU] EMI_WP_ADR = 0x%x\n", readl(IOMEM(EMI_WP_ADR)));
	pr_err("[EMI MPU] EMI_WP_CTRL = 0x%x\n", readl(IOMEM(EMI_WP_CTRL)));
	pr_err("[EMI MPU] EMI_CHKER = 0x%x\n", readl(IOMEM(EMI_CHKER)));
	pr_err("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n",
	readl(IOMEM(EMI_CHKER_TYPE)));
	pr_err("[EMI MPU] EMI_CHKER_ADR = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR)));

	__clear_emi_mpu_vio();


	/* Set Device APC initialization for EMI-MPU. */
	mt_devapc_emi_initial();

	if (1) /* (0 == enable_4G()) there is on 4G mode of MT6735~MT6753*/ {
		emi_physical_offset = 0x40000000;
		pr_err("[EMI MPU] Not 4G mode\n");
	} else {
		/*enable 4G mode*/
		emi_physical_offset = 0;
		pr_err("[EMI MPU] 4G mode\n");
	}

	/*
	 * NoteXXX: Interrupts of violation (including SPC in SMI, or EMI MPU)
	 *          are triggered by the device APC.
	 *          Need to share the interrupt with the SPC driver.
	 */

	ret = request_irq(mpu_irq,
			  (irq_handler_t)mpu_violation_irq,
			  IRQF_TRIGGER_LOW | IRQF_SHARED,
			  "mt_emi_mpu",
			  &emi_mpu_ctrl);
	if (ret != 0) {
		pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
		return ret;
	}

	protect_ap_region();

#if defined(CONFIG_ARCH_MT6753)
	acquire_dram_setting(&DRAM_setting);
	pr_err("[EMI] EMI_CONA =  0x%x\n", readl(IOMEM((EMI_CONA))));
	pr_err("[EMI] Support channel number %d\n", DRAM_setting.channel_nr);
	pr_err("[EMI] Channel 0 : rank 0 : %d Gb, segment no : %d\n",
	       DRAM_setting.channel[0].rank[0].rank_size,
	       DRAM_setting.channel[0].rank[0].segment_nr);
	pr_err("[EMI] Channel 0 : rank 1 : %d Gb, segment no : %d\n",
	       DRAM_setting.channel[0].rank[1].rank_size,
	       DRAM_setting.channel[0].rank[1].segment_nr);
	pr_err("[EMI] Channel 1 : rank 0 : %d Gb, segment no : %d\n",
	       DRAM_setting.channel[1].rank[0].rank_size,
	       DRAM_setting.channel[1].rank[0].segment_nr);
	pr_err("[EMI] Channel 1 : rank 1 : %d Gb, segment no : %d\n",
	       DRAM_setting.channel[1].rank[1].rank_size,
	       DRAM_setting.channel[1].rank[1].segment_nr);
#endif

#ifdef ENABLE_EMI_CHKER
	/* AXI violation monitor setting and timer function create */
	mt_reg_sync_writel((1 << AXI_VIO_CLR) | readl(IOMEM(EMI_CHKER)),
	EMI_CHKER);
	emi_axi_set_master(MASTER_ALL);
	init_timer(&emi_axi_vio_timer);
	emi_axi_vio_timer.expires = jiffies + AXI_VIO_MONITOR_TIME;
	emi_axi_vio_timer.function = &emi_axi_vio_timer_func;
	emi_axi_vio_timer.data = ((unsigned long) 0);
#endif /*#ifdef ENABLE_EMI_CHKER*/

#if !defined(USER_BUILD_KERNEL)
#ifdef ENABLE_EMI_CHKER
	/* Enable AXI 4KB boundary violation monitor timer */
	/*emi_axi_set_chker(1 << AXI_ADR_CHK_EN);*/
	/*add_timer_on(&emi_axi_vio_timer, 0);*/
#endif

	/* register driver and create sysfs files */
	ret = platform_driver_register(&emi_mpu_ctrl);
	if (ret)
		pr_err("Fail to register EMI_MPU driver.\n");

	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_mpu_config);
	if (ret)
		pr_err("Fail to create MPU config sysfs file.\n");

#ifdef ENABLE_EMI_CHKER
	ret = driver_create_file(&emi_mpu_ctrl.driver,
	&driver_attr_emi_axi_vio);
	if (ret)
		pr_err("Fail to create AXI violation monitor sysfs file.\n");

#endif
	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_pgt_scan);
	if (ret)
		pr_err("Fail to create pgt scan sysfs file.\n");

#ifdef ENABLE_EMI_WATCH_POINT
	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_emi_wp_vio);
	if (ret)
		pr_err("Fail to create WP violation monitor sysfs file.\n");

#endif
#endif

	/*atomic_notifier_chain_register(&panic_notifier_list, &emi_mpu_blk);*/

	/* Create a workqueue to search pagetable entry */
	emi_mpu_workqueue = create_singlethread_workqueue("emi_mpu");
	INIT_WORK(&emi_mpu_work, emi_mpu_work_callback);
	return 0;
}

static void __exit emi_mpu_mod_exit(void)
{
}

/*
  unsigned int enable_4G(void)
  {
  return 0;
  }
*/
module_init(emi_mpu_mod_init);
module_exit(emi_mpu_mod_exit);

/*EXPORT_SYMBOL(start_mm_mau_protect);*/
