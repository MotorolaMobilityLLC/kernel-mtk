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

#include <mt-plat/mtk_device_apc.h>
#include <mt-plat/sync_write.h>
/* #include "mach/irqs.h" */
#include <mt-plat/dma.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <mt-plat/mtk_io.h>
#include "mach/emi_mpu.h"
#include <mt-plat/mtk_ccci_common.h>
#include <linux/delay.h>

static int Violation_PortID = MASTER_ALL;

#define EMI_MPU_TEST	1
#if EMI_MPU_TEST
char mpu_test_buffer[0x20000] __aligned(PAGE_SIZE);
#endif

#define ENABLE_EMI_CHKER
#define ENABLE_EMI_WATCH_POINT

#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define TIMEOUT 100
#define AXI_VIO_MONITOR_TIME	(1 * HZ)

static struct work_struct emi_mpu_work;
static struct workqueue_struct *emi_mpu_workqueue;
static void __iomem *EMI_BASE_ADDR; /* not initialise statics to 0 or NULL */
void __iomem *CHN0_EMI_BASE_ADDR;
void __iomem *CHN1_EMI_BASE_ADDR;
void __iomem *DRAMC_CH0_TOP0_BASE_ADDR;
void __iomem *DRAMC_CH1_TOP0_BASE_ADDR;
void __iomem *INFRA_BASE_ADDR;
void __iomem *INFRACFG_BASE_ADDR;
static unsigned int enable_4gb;
static unsigned long long vio_addr;
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
	{ .master = MST_ID_APMCU_0,  .port = 0x0, .id_mask = 0x1FFC,
		.id_val = 0x0,
		.note = "CA7LL: Core nn system domain store exclusive",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_1,  .port = 0x0, .id_mask = 0x1FFC,
		.id_val = 0x4,
		.note = "CA7LL: Core nn barrier",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_2,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x8,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_3,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x9,
		.note = "CA7LL: SCU generated barrier",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_4,  .port = 0x0, .id_mask = 0x1FFE,
		.id_val = 0xA,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_5,  .port = 0x0, .id_mask = 0x1FFC,
		.id_val = 0xC,
		.note = "CA7LL: Core nn non-re-orderable device write",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_6,  .port = 0x0, .id_mask = 0x1FF0,
		.id_val = 0x10,
		.note = "CA7LL: Write to normal memory",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_7,  .port = 0x0, .id_mask = 0x1FFC,
		.id_val = 0x0,
		.note = "CA7LL: Core nn exclusive read",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_8,  .port = 0x0, .id_mask = 0x1FFC,
		.id_val = 0x4,
		.note = "CA7LL: Core nn barrier",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_9,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x8,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_10,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x9,
		.note = "CA7LL: SCU generated barrier or DVM complete",
		.name = "CA53_m_39" },
	{ .master = MST_ID_APMCU_11,  .port = 0x0, .id_mask = 0x1FFE,
		.id_val = 0xA,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39"},
	{ .master = MST_ID_APMCU_12,  .port = 0x0, .id_mask = 0x1FFC,
		.id_val = 0xC,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39"},
	{ .master = MST_ID_APMCU_13,  .port = 0x0, .id_mask = 0x1FF3,
		.id_val = 0x10,
		.note = "CA7LL: ACP read",
		.name = "CA53_m_39"},
	{ .master = MST_ID_APMCU_14,  .port = 0x0, .id_mask = 0x1FF3,
		.id_val = 0x11,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39"},
	{ .master = MST_ID_APMCU_15,  .port = 0x0, .id_mask = 0x1FF2,
		.id_val = 0x12,
		.note = "CA7LL: Unused",
		.name = "CA53_m_39"},
	{ .master = MST_ID_APMCU_16,  .port = 0x0, .id_mask = 0x1FE0,
		.id_val = 0x20,
		.note = "CA7LL: Core nn read",
		.name = "CA53_m_39" },

	/* MM */
	{ .master = MST_ID_MM_0, .port = 0x1, .id_mask = 0x1F80,
		.id_val = 0x0, .note = "smi_larb0",
		.name = "smi_larb0_m_39" },
	{ .master = MST_ID_MM_1, .port = 0x1, .id_mask = 0x1F80,
		.id_val = 0x80, .note = "smi_larb1",
		.name = "smi_larb1_m_39" },
	{ .master = MST_ID_MM_2, .port = 0x1, .id_mask = 0x1F80,
		.id_val = 0x100, .note = "smi_larb2",
		.name = "smi_larb2_m_39" },
	{ .master = MST_ID_MM_3, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x3FC, .note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU_Internal_Used_m_39" },
	{ .master = MST_ID_MM_4, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x3FD, .note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU_Internal_Used_m_39" },

	/* INFRA */
	{ .master = MST_ID_INFRA_0, .port = 0x2, .id_mask = 0x1FF7,
		.id_val = 0x0, .note = "DebugTop",
		.name = "infra_debug_top_m_39" },
	{ .master = MST_ID_INFRA_1, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x1, .note = "MSDC0",
		.name = "infra_msdc0_m_39" },
	{ .master = MST_ID_INFRA_2, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x9, .note = "PWM",
		.name = "infra_pwm_m_39" },
	{ .master = MST_ID_INFRA_3, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x49, .note = "MSDC1",
		.name = "infra_msdc1_m_39" },
	{ .master = MST_ID_INFRA_4, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x89, .note = "Audio",
		.name = "infra_audio_m_39" },
	{ .master = MST_ID_INFRA_5, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0xC9, .note = "SPI0",
		.name = "infra_spi_m_39" },
	{ .master = MST_ID_INFRA_6, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x11, .note = "NFI",
		.name = "infra_nfi_m_39" },
	{ .master = MST_ID_INFRA_7, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x51, .note = "SPI2",
		.name = "infra_spi_m_39" },
	{ .master = MST_ID_INFRA_8, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x91, .note = "SPI1",
		.name = "infra_spi_m_39" },
	{ .master = MST_ID_INFRA_9, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0xD1, .note = "USB2.0",
		.name = "infra_usb_m_39" },
	{ .master = MST_ID_INFRA_10, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x19, .note = "MCUPM",
		.name = "infra_mcupm_m_39" },
	{ .master = MST_ID_INFRA_11, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x59, .note = "SPM",
		.name = "infra_spm_m_39" },
	{ .master = MST_ID_INFRA_12, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x99, .note = "MD",
		.name = "infra_md_m_39" },
	{ .master = MST_ID_INFRA_13, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0xD9, .note = "THERM",
		.name = "infra_therm_m_39" },
	{ .master = MST_ID_INFRA_14, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x21, .note = "DMA_EXT",
		.name = "infra_dma_ext_m_39" },
	{ .master = MST_ID_INFRA_15, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x2, .note = "CONNSYS",
		.name = "infra_connsys_m_39" },
	{ .master = MST_ID_INFRA_16, .port = 0x2, .id_mask = 0x1F87,
		.id_val = 0x3, .note = "DX_CC",
		.name = "infra_dx_cc_m_39" },
	{ .master = MST_ID_INFRA_17, .port = 0x2, .id_mask = 0x1F87,
		.id_val = 0x4, .note = "CQDMA",
		.name = "infra_cqdma_m_39" },
	{ .master = MST_ID_INFRA_18, .port = 0x2, .id_mask = 0x1FE7,
		.id_val = 0x5, .note = "CLDMA",
		.name = "infra_cldma_m_39" },
	{ .master = MST_ID_INFRA_19, .port = 0x2, .id_mask = 0x1FE7,
		.id_val = 0x6, .note = "GCE_M",
		.name = "infra_gce_m_39" },

	/* Modem (EMI port 3) */
	{ .master = MST_ID_MD_IA_0,    .port = 0x3, .id_mask = 0x1F83,
		.id_val = 0x0, .note = "MD_MM",
		.name = "MD_IA_m_39" },
	{ .master = MST_ID_MD_IA_1,    .port = 0x3, .id_mask = 0x1F83,
		.id_val = 0x1, .note = "MD MMU",
		.name = "MD_IA_m_39" },
	{ .master = MST_ID_MD_USIP_0,    .port = 0x3, .id_mask = 0x1F1F,
		.id_val = 0x2, .note = "usip_0_i",
		.name = "MD_USIP_m_39" },
	{ .master = MST_ID_MD_USIP_1,    .port = 0x3, .id_mask = 0x1F1F,
		.id_val = 0x6, .note = "usip_0_dcache",
		.name = "MD_USIP_m_39" },
	{ .master = MST_ID_MD_USIP_2,    .port = 0x3, .id_mask = 0x1F1F,
		.id_val = 0xA, .note = "usip_0_dnocache",
		.name = "MD_USIP_m_39" },
	{ .master = MST_ID_MD_USIP_3,    .port = 0x3, .id_mask = 0x1F1F,
		.id_val = 0x12, .note = "usip_1_i",
		.name = "MD_USIP_m_39" },
	{ .master = MST_ID_MD_USIP_4,    .port = 0x3, .id_mask = 0x1F1F,
		.id_val = 0x16, .note = "usip_1_dcache",
		.name = "MD_USIP_m_39" },
	{ .master = MST_ID_MD_USIP_5,    .port = 0x3, .id_mask = 0x1F1F,
		.id_val = 0x1A, .note = "usip_1_dnocache",
		.name = "MD_USIP_m_39" },

	/* Modem (EMI port 4) */
	{ .master = MST_ID_MD_RXBRP_0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0, .note = "hrq_rd",
		.name = "MD_RXBRP_m_39" },
	{ .master = MST_ID_MD_RXBRP_1, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x804, .note = "hrq_rd1",
		.name = "MD_RXBRP_m_39" },
	{ .master = MST_ID_MD_RXBRP_2, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x801, .note = "hrq_wr",
		.name = "MD_RXBRP_m_39" },
	{ .master = MST_ID_MD_RXBRP_3, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x805, .note = "hrq_wr1",
		.name = "MD_RXBRP_m_39" },
	{ .master = MST_ID_MD_RXBRP_4, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x806, .note = "vtb",
		.name = "MD_RXBRP_m_39" },
	{ .master = MST_ID_MD_RXBRP_5, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x80A, .note = "tbo",
		.name = "MD_RXBRP_m_39" },
	{ .master = MST_ID_MD_RXBRP_6, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x802, .note = "debug",
		.name = "MD_RXBRP_m_39" },

	{ .master = MST_ID_MD_Bigram_0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x404, .note = "dfe_dump",
		.name = "MD_Bigram_m_39" },
	{ .master = MST_ID_MD_Bigram_1, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x405, .note = "br_dma",
		.name = "MD_Bigram_m_39" },

	{ .master = MST_ID_MD_MD2G_0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x8, .note = "irdma",
		.name = "MD_MD2G_m_39" },

	{ .master = MST_ID_MD_TXBRP_0,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x28, .note = "txbrp0",
		.name = "MD_TXBRP_m_39" },
	{ .master = MST_ID_MD_TXBRP_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xA8, .note = "txbrp1",
		.name = "MD_TXBRP_m_39" },

	{ .master = MST_ID_MD_TXDFE_0,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x68, .note = "txcal",
		.name = "MD_TXDFE_m_39" },
	{ .master = MST_ID_MD_TXDFE_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xE8, .note = "TPC",
		.name = "MD_TXDFE_m_39" },

	{ .master = MST_ID_MD_RXDFESYS_0,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x38, .note = "RXDFE_DMA",
		.name = "MD_RXDFESYS_m_39" },
	{ .master = MST_ID_MD_RXDFESYS_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x78, .note = "MRSG0",
		.name = "MD_RXDFESYS_m_39" },
	{ .master = MST_ID_MD_RXDFESYS_2,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xB8, .note = "MRSG1",
		.name = "MD_RXDFESYS_m_39" },

	{ .master = MST_ID_MD_CS_0,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x98, .note = "CNWDMA",
		.name = "MD_CS_m_39" },
	{ .master = MST_ID_MD_CS_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x58, .note = "CSH",
		.name = "MD_CS_m_39" },
	{ .master = MST_ID_MD_CS_2,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x18, .note = "DCXO",
		.name = "MD_CS_m_39" },

	{ .master = MST_ID_MD_mml2_0,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xC02, .note = "DMA_RD",
		.name = "MD_mml2_m_39" },
	{ .master = MST_ID_MD_mml2_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xC03, .note = "DMA_WR",
		.name = "MD_mml2_m_39" },
	{ .master = MST_ID_MD_mml2_2,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xC00, .note = "MMU",
		.name = "MD_mml2_m_39" },
	{ .master = MST_ID_MD_mml2_3,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0xC01, .note = "QP",
		.name = "MD_mml2_m_39" },

	{ .master = MST_ID_MD_mdinfra_0,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x12, .note = "LOG_TOP_MCU",
		.name = "MD_mdinfra_m_39" },
	{ .master = MST_ID_MD_mdinfra_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x3, .note = "LOG_TOP_DSP",
		.name = "MD_mdinfra_m_39" },
	{ .master = MST_ID_MD_mdinfra_2,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x4, .note = "TRACE_TOP",
		.name = "MD_mdinfra_m_39" },
	{ .master = MST_ID_MD_mdinfra_3,   .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x1, .note = "PPPHA",
		.name = "MD_mdinfra_m_39" },
	{ .master = MST_ID_MD_mdinfra_4,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x5, .note = "IPSEC",
		.name = "MD_mdinfra_m_39" },

	{ .master = MST_ID_MD_MDPERI_0,   .port = 0x4, .id_mask = 0x1F1F,
		.id_val = 0x7, .note = "GDMA",
		.name = "MD_MDPERI_m_39" },
	{ .master = MST_ID_MD_MDPERI_1,   .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x7, .note = "DBGSYS",
		.name = "MD_MDPERI_m_39" },

	/* MFG */
	{ .master = MST_ID_MFG_0, .port = 0x5, .id_mask = 0x1FC0,
		.id_val = 0x0, .note = "MFG", .name = "MFG_m_39" },

};

static const char *UNKNOWN_MASTER = "unknown";
static spinlock_t emi_mpu_lock;

#ifdef ENABLE_EMI_CHKER
struct timer_list emi_axi_vio_timer;
#endif

char *smi_larb0_port[8] = {
	"LARB0_display_m_57", "LARB0_display_m_57", "LARB0_display_m_57",
	"LARB0_display_m_57", "LARB0_MDP_m_57", "LARB0_MDP_m_57",
	"LARB0_MDP_m_57", "LARB0_display_m_57"};
char *smi_larb1_port[7] =  {
	"LARB1_VDEC_m_57", "LARB1_VDEC_m_57", "LARB1_VDEC_m_57",
	"LARB1_VDEC_m_57", "LARB1_VDEC_m_57", "LARB1_VDEC_m_57",
	"LARB1_VDEC_m_57"};
char *smi_larb2_port[17] = {
	"LARB2_CAM_m_57", "LARB2_CAM_m_57", "LARB2_CAM_m_57",
	"LARB2_CAM_m_57", "LARB2_CAM_m_57", "LARB2_CAM_m_57",
	"LARB2_CAM_m_57", "LARB2_CAM_SV_m_57", "LARB2_CAM_SV_m_57",
	"LARB2_CAM_SV_m_57", "LARB2_CAM_m_57", "LARB2_CAM_m_57",
	"LARB2_CAM_m_57", "LARB2_CAM_m_57", "LARB2_CAM_m_57",
	"LARB2_CAM_CCU_m_57", "LARB2_CAM_CCU_m_57"};

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID)
{
	u32 mm_larb;
	u32 smi_port;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val)
		&& (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case 0: /* MASTER_APMCU */
		case 2: /* INFRA */
		case 3:	/* MASTER_MDMCU */
		case 4:	/* MASTER_MDHW */
		case 5:	/* MASTER_MFG0 */
			pr_debug("Violation master name is %s.\n",
			mst_tbl[tbl_idx].name);
			break;
		case 1: /* MASTER_MM */
			 /*MM*/ mm_larb = axi_id >> 7;
			smi_port = (axi_id & 0x7F) >> 2;
			if (mm_larb == 0x0) {	/* smi_larb0 */
				if (smi_port >= ARRAY_SIZE(smi_larb0_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_debug("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb0_port[smi_port]);
			} else if (mm_larb == 0x1) {	/* smi_larb1 */
				if (smi_port >= ARRAY_SIZE(smi_larb1_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_debug("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb1_port[smi_port]);
			} else if (mm_larb == 0x2) {	/* smi_larb2 */
				if (smi_port >= ARRAY_SIZE(smi_larb2_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_debug("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb2_port[smi_port]);
			} else {	/*MM IOMMU Internal Used */
				pr_debug("Violation master name is %s.\n",
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

#if 0
static u32 __id2mst(u32 id)
{
	int i;
	u32 axi_ID;
	u32 port_ID;

	axi_ID = (id >> 3) & 0x000001FFF;
	port_ID = id & 0x00000007;

	pr_debug("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))
			return mst_tbl[i].master;
	}
	return MST_INVALID;
}
#endif

static int isetocunt;
static void emi_mpu_set_violation_port(int portID)
{
	if (isetocunt == 0) {
		Violation_PortID = portID;
		isetocunt = 1;
	} else if (isetocunt == 2) {
		isetocunt = 0;
	}
}

int emi_mpu_get_violation_port(void)
{
	int ret;

	pr_debug("[EMI MPU] emi_mpu_get_violation_port\n");

	if (isetocunt == 0) {
		pr_err("[EMI MPU] EMI did not set Port ID\n");
		isetocunt = 2;
		return MASTER_ALL;
	}

	ret = Violation_PortID;
	isetocunt = 0;

	if (ret >= MASTER_ALL)
		pr_err("[EMI MPU] Port ID: %d is an invalid Port ID\n", ret);
	else
		Violation_PortID = MASTER_ALL;

	return ret;
}

static char *__id2name(u32 id)
{
	int i;
	u32 axi_ID;
	u32 port_ID;
	u32 mm_larbID;
	u32 smi_portID;

	axi_ID = (id >> 3) & 0x0000FFF;
	port_ID = id & 0x00000007;
	emi_mpu_set_violation_port(port_ID);
	pr_debug("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))	{
			if (port_ID == 1) {
				mm_larbID = axi_ID >> 7;
				smi_portID = (axi_ID & 0x7F) >> 2;
				if (mm_larbID == 0x0)
					return smi_larb0_port[smi_portID];
				else if (mm_larbID == 0x1)
					return smi_larb1_port[smi_portID];
				else if (mm_larbID == 0x2)
					return smi_larb2_port[smi_portID];
				else
					return mst_tbl[i].name;
			} else
				return mst_tbl[i].name;
		}
	}

	return (char *)UNKNOWN_MASTER;
}

static void __clear_emi_mpu_vio(unsigned int first)
{
	u32 dbg_s, dbg_t, i;

	/* clear violation status */
	for (i = 0; i < EMI_MPU_DOMAIN_NUMBER; i++) {
		mt_reg_sync_writel(0xFFFFFFFF, EMI_MPUD_ST(i));  /* Region abort violation */
		mt_reg_sync_writel(0x3, EMI_MPUD_ST2(i));  /* Out of region abort violation */
	}

	/* clear debug info */
	mt_reg_sync_writel(0x80000000, EMI_MPUS);
	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));

	if (first) {
		/* clear the permission setting of MPU*/
		/* do not need to clear here, we already do it at preloader */
	}

	if (dbg_s) {
		pr_debug("Fail to clear EMI MPU violation\n");
		pr_debug("EMI_MPUS = %x, EMI_MPUT = %x", dbg_s, dbg_t);
	}
}

static int mpu_check_violation(void)
{
	u32 dbg_s, dbg_t, dbg_t_2nd, dbg_pqry;
	u32 master_ID, domain_ID, wr_vio, wr_oo_vio;
	s32 region;
	char *master_name;

	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));
	dbg_t_2nd = readl(IOMEM(EMI_MPUT_2ND));
	vio_addr = (dbg_t + (((unsigned long long)(dbg_t_2nd & 0xF)) << 32) + emi_physical_offset);

	pr_debug("Clear status.\n");

	master_ID = (dbg_s & 0x0000FFFF);
	domain_ID = (dbg_s >> 21) & 0x0000000F;
	wr_vio = (dbg_s >> 29) & 0x00000003;
	wr_oo_vio = (dbg_s >> 27) & 0x00000003;
	region = (dbg_s >> 16) & 0x1F;
	dbg_pqry = readl(IOMEM(EMI_MPUD_ST(domain_ID)));

	/*TBD: print the abort region*/

	pr_debug("EMI MPU violation.\n");
	pr_debug("[EMI MPU] Debug info start ----------------------------------------\n");

	pr_debug("EMI_MPUS = %x, EMI_MPUT = %x, EMI_MPUT_2ND = %x.\n", dbg_s, dbg_t, dbg_t_2nd);
	pr_debug("Current process is \"%s \" (pid: %i).\n",
	current->comm, current->pid);
	pr_debug("Violation address is 0x%llx.\n", vio_addr);
	pr_debug("Violation master ID is 0x%x.\n", master_ID);
	/*print out the murderer name*/
	master_name = __id2name(master_ID);
	pr_debug("Violation domain ID is 0x%x.\n", domain_ID);
	if (wr_vio == 1)
		pr_debug("Write violation.\n");
	else if (wr_vio == 2)
		pr_debug("Read violation.\n");
	else
		pr_debug("Strange write / read violation value = %d.\n", wr_vio);
	pr_debug("Corrupted region is %d\n\r", region);
	if (wr_oo_vio == 1)
		pr_debug("Write out of range violation.\n");
	else if (wr_oo_vio == 2)
		pr_debug("Read out of range violation.\n");

	pr_debug("[EMI MPU] Debug info end------------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
	if (wr_vio != 0) {
	/* EMI violation is relative to MD at user build*/
	#if 0 /* #ifndef CONFIG_MTK_ENG_BUILD */
		if (((master_ID & 0x7) == MASTER_MDMCU) ||
			((master_ID & 0x7) == MASTER_MDHW)) {
			int md_id = 0;

			exec_ccci_kern_func_by_md_id(md_id,
			ID_FORCE_MD_ASSERT, NULL, 0);
			pr_debug("[EMI MPU] MPU violation trigger MD\n");
		}
	#else
		if (((master_ID & 0x7) == MASTER_MDMCU) ||
			((master_ID & 0x7) == MASTER_MDHW)) {
			char str[60] = "0";
			char *pstr = str;

			sprintf(pstr, "EMI_MPUS = 0x%x, ADDR = 0x%llx",
				dbg_s, vio_addr);

			exec_ccci_kern_func_by_md_id(0,
			ID_MD_MPU_ASSERT, str, strlen(str));
			pr_debug("[EMI MPU] MPU violation trigger MD str=%s strlen(str)=%d\n"
				, str, (int)strlen(str));
		} else {
			exec_ccci_kern_func_by_md_id(0,
			ID_MD_MPU_ASSERT, NULL, 0);
			pr_debug("[EMI MPU] MPU violation ack to MD\n");
		}
	#endif
		if ((region == 0) && (mt_emi_reg_read(EMI_MPU_SA(0)) == 0)
			&& (mt_emi_reg_read(EMI_MPU_EA(0)) == 0)
			&& (mt_emi_reg_read(EMI_MPU_APC(0, 0)) == 0)
			&& (!(dbg_pqry & OOR_VIO))) {
			pr_debug("[EMI MPU] A strange violation.\n");
		} else {
			aee_kernel_exception("EMI MPU",
"%sEMI_MPUS = 0x%x,EMI_MPUT = 0x%x,EMI_MPUT_2ND = 0x%x,vio_addr = 0x%llx\n CHKER = 0x%x,CHKER_TYPE = 0x%x,CHKER_ADR = 0x%x\n%s%s\n",
				     "EMI MPU violation.\n",
				     dbg_s,
				     dbg_t,
				     dbg_t_2nd,
				     vio_addr,
				     readl(IOMEM(EMI_CHKER)),
				     readl(IOMEM(EMI_CHKER_TYPE)),
				     readl(IOMEM(EMI_CHKER_ADR)),
				     "CRDISPATCH_KEY:EMI MPU Violation Issue/",
				     master_name);
		}
	}
#endif

	__clear_emi_mpu_vio(0);
	/* mt_devapc_clear_emi_violation(); */
	return 0;

}

/*EMI MPU violation handler*/
static irqreturn_t mpu_violation_irq(int irq, void *dev_id)
{
	pr_info("It's a MPU violation.\n");
	mpu_check_violation();
	return IRQ_HANDLED;
}

#if defined(CONFIG_MTK_PASR)
/* Acquire DRAM Setting for PASR/DPD */
void acquire_dram_setting(struct basic_dram_setting *pasrdpd)
{
	int ch_nr = MAX_CHANNELS;
	unsigned int emi_cona, emi_conh, col_bit, row_bit;
	unsigned int ch0_rank0_size, ch0_rank1_size;
	unsigned int ch1_rank0_size, ch1_rank1_size;
	unsigned int shift_for_16bit = 1;

	pasrdpd->channel_nr = ch_nr;

	emi_cona = readl(IOMEM(EMI_CONA));
	emi_conh = readl(IOMEM(EMI_CONH));

	/* Is it 32-bit or 16-bit I/O */
	if (emi_cona & 0x2)
		shift_for_16bit = 0;

	ch0_rank0_size = (emi_conh >> 16) & 0xf;
	ch0_rank1_size = (emi_conh >> 20) & 0xf;
	ch1_rank0_size = (emi_conh >> 24) & 0xf;
	ch1_rank1_size = (emi_conh >> 28) & 0xf;

	{
		pasrdpd->channel[0].rank[0].valid_rank = true;

		if (ch0_rank0_size == 0) {
			col_bit = ((emi_cona >> 4) & 0x03) + 9;
			row_bit = (((emi_cona >> 24) & 0x1) << 2) + ((emi_cona >> 12) & 0x03) + 13;
			pasrdpd->channel[0].rank[0].rank_size =
			(1 << (row_bit + col_bit)) >> (22 + shift_for_16bit);
			pasrdpd->channel[0].rank[0].segment_nr = 8;
		} else {
			pasrdpd->channel[0].rank[0].rank_size =
			(ch0_rank0_size * 2);
			pasrdpd->channel[0].rank[0].segment_nr = 6;
		}

		if (0 != (emi_cona & (1 << 17))) {
			pasrdpd->channel[0].rank[1].valid_rank = true;

			if (ch0_rank1_size == 0) {
				col_bit = ((emi_cona >> 6) & 0x03) + 9;
				row_bit = (((emi_cona >> 25) & 0x1) << 2) + ((emi_cona >> 14) & 0x03) + 13;
				pasrdpd->channel[0].rank[1].rank_size =
				(1 << (row_bit + col_bit)) >> (22 + shift_for_16bit);
				pasrdpd->channel[0].rank[1].segment_nr = 8;
			} else {
				pasrdpd->channel[0].rank[1].rank_size =
				(ch0_rank1_size * 2);
				pasrdpd->channel[0].rank[1].segment_nr = 6;
			}
		} else {
			pasrdpd->channel[0].rank[1].valid_rank = false;
			pasrdpd->channel[0].rank[1].segment_nr = 0;
			pasrdpd->channel[0].rank[1].rank_size = 0;
		}
	}

	if (0 != ((emi_cona >> 8) & 0x01)) {

		pasrdpd->channel[1].rank[0].valid_rank = true;

		if (ch1_rank0_size == 0) {
			col_bit = ((emi_cona >> 20) & 0x03) + 9;
			row_bit = (((emi_conh >> 4) & 0x1) << 2) + ((emi_cona >> 28) & 0x03) + 13;
			pasrdpd->channel[1].rank[0].rank_size =
			(1 << (row_bit + col_bit)) >> (22 + shift_for_16bit);
			pasrdpd->channel[1].rank[0].segment_nr = 8;
		} else {
			pasrdpd->channel[1].rank[0].rank_size =
			(ch1_rank0_size * 2);
			pasrdpd->channel[1].rank[0].segment_nr = 6;
		}

		if (0 != (emi_cona & (1 << 16))) {
			pasrdpd->channel[1].rank[1].valid_rank = true;

			if (ch1_rank1_size == 0) {
				col_bit = ((emi_cona >> 22) & 0x03) + 9;
				row_bit = (((emi_conh >> 5) & 0x1) << 2) + ((emi_cona >> 30) & 0x03) + 13;
				pasrdpd->channel[1].rank[1].rank_size =
				(1 << (row_bit + col_bit)) >> (22 + shift_for_16bit);
				pasrdpd->channel[1].rank[1].segment_nr = 8;
			} else {
				pasrdpd->channel[1].rank[1].rank_size =
				(ch1_rank1_size * 2);
				pasrdpd->channel[1].rank[1].segment_nr = 6;
			}
		} else {
			pasrdpd->channel[1].rank[1].valid_rank = false;
			pasrdpd->channel[1].rank[1].segment_nr = 0;
			pasrdpd->channel[1].rank[1].rank_size = 0;
		}
	} else {
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
int emi_mpu_set_region_protection(unsigned long long start,
unsigned long long end, int region, unsigned int access_permission)
{
	int ret = 0;
	unsigned long flags;

	access_permission = access_permission & 0xFFFFFF;
	access_permission = access_permission | ((region & 0x1F)<<27);
	spin_lock_irqsave(&emi_mpu_lock, flags);
	mt_emi_mpu_set_region_protection(start, end, access_permission);
	spin_unlock_irqrestore(&emi_mpu_lock, flags);

	return ret;
}
EXPORT_SYMBOL(emi_mpu_set_region_protection);

static ssize_t emi_mpu_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	unsigned long long start, end;
	unsigned int reg_value;
	int i, j, region;
	static const char *permission[7] = {
		"No",
		"S_RW",
		"S_RW_NS_R",
		"S_RW_NS_W",
		"S_R_NS_R",
		"FORBIDDEN",
		"S_R_NS_RW"
	};

	for (region = 0; region < EMI_MPU_REGION_NUMBER; region++) {
		start = (unsigned long long)(mt_emi_reg_read(EMI_MPU_SA(region)) << 16) + emi_physical_offset;
		end = (unsigned long long)(mt_emi_reg_read(EMI_MPU_EA(region)) << 16) + emi_physical_offset;
		ptr += sprintf(ptr, "R%d-> 0x%llx to 0x%llx\n", region, start, end+0xFFFF);

		ptr += sprintf(ptr, "R%d->\n", region);
		/* for(i=0; i<(EMI_MPU_DOMAIN_NUMBER/8); i++) { */
		/* only print first 8 domain */
		for (i = 0; i < 1; i++) {
			reg_value = mt_emi_reg_read(EMI_MPU_APC(region, i*8));
			for (j = 0; j < 8; j++) {
				ptr += sprintf(ptr, "d%d = %s, ", i*8+j, permission[((reg_value >> (j*3)) & 0x7)]);
				if ((j == 3) || (j == 7))
					ptr += sprintf(ptr, "\n");
			}
		}
		ptr += sprintf(ptr, "\n");
	}


#if EMI_MPU_TEST
	{
		int temp;

		temp = (*((volatile unsigned int *)(mpu_test_buffer+0x10000)));
		pr_debug("mpu_test_buffer+10000 = 0x%x\n", temp);
	}
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
	char *token[6]; /* token[6] for NULL */
	ssize_t ret;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_mpu_store command overflow.");
		return count;
	}
	pr_debug("emi_mpu_store: %s\n", buf);

	command = kmalloc((size_t) MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strcpy(command, buf);
	ptr = (char *)buf;

	if (!strncmp(buf, EN_MPU_STR, strlen(EN_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		for (i = 0; i < 5; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);

		/* kernel standardization
		 * start_addr = simple_strtoul(token[1], &token[1], 16);
		 * end_addr = simple_strtoul(token[2], &token[2], 16);
		 * region = simple_strtoul(token[3], &token[3], 16);
		 * access_permission = simple_strtoul(token[4], &token[4], 16);
		 */
		ret = kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse start_addr");
		ret = kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse end_addr");
		ret = kstrtoul(token[3], 10, (unsigned long *)&region);
		if (ret != 0)
			pr_err("kstrtoul fails to parse region");
		ret = kstrtoul(token[4], 16,
		(unsigned long *)&access_permission);
		if (ret != 0)
			pr_err("kstrtoul fails to parse access_permission");
		emi_mpu_set_region_protection(start_addr, end_addr,
		region, access_permission);
		pr_debug("Set EMI_MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x.\n",
		       start_addr, end_addr, region, access_permission);
	} else if (!strncmp(buf, DIS_MPU_STR, strlen(DIS_MPU_STR))) {
		i = 0;
		while (ptr != NULL) {
			ptr = strsep(&command, " ");
			token[i] = ptr;
			pr_devel("token[%d] = %s\n", i, token[i]);
			i++;
		}
		/* kernel standardization
		 * start_addr = simple_strtoul(token[1], &token[1], 16);
		 * end_addr = simple_strtoul(token[2], &token[2], 16);
		 * region = simple_strtoul(token[3], &token[3], 16);
		 */
		ret = kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse start_addr");
		ret = kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse end_addr");
		ret = kstrtoul(token[3], 10, (unsigned long *)&region);
		if (ret != 0)
			pr_err("kstrtoul fails to parse region");

		emi_mpu_set_region_protection(0x0, 0x0, region,
		SET_ACCESS_PERMISSON(NO_PROTECTION, NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION));

		pr_debug("set EMI MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x\n",
		0, 0, region,
		SET_ACCESS_PERMISSON(NO_PROTECTION,	NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION,
		NO_PROTECTION, NO_PROTECTION));

	} else {
		pr_err("Unknown emi_mpu command.\n");
	}

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
	unsigned long long v_addr = vio_addr;

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
					pr_debug("[EMI MPU] Find page entry section at pte: %lx. violation address = 0x%llx\n",
					(unsigned long)(pte), v_addr);
					return;
				}
			}
		} else {
			/* Section */
			if (((unsigned long)pmd_val(*(pmd)) & SECTION_MASK) ==
				((unsigned long)v_addr
				& SECTION_MASK)) {
				pr_debug("[EMI MPU] Find page entry section at pmd: %lx. violation address = 0x%llx\n",
				(unsigned long)(pmd), v_addr);
				return;
			}
		}
#else
		/* TBD */
#endif
	}
	pr_debug("[EMI MPU] ****** Can not find page table entry! violation address = 0x%llx ******\n",
	v_addr);
}

void emi_mpu_work_callback(struct work_struct *work)
{
	pr_debug("[EMI MPU] Enter EMI MPU workqueue!\n");
	mtk_search_full_pgtab();
	pr_debug("[EMI MPU] Exit EMI MPU workqueue!\n");
}

static ssize_t pgt_scan_show(struct device_driver *driver, char *buf)
{
	dump_emi_latency();
	dump_emi_MM();
	return 0;
}

static ssize_t pgt_scan_store(struct device_driver *driver,
const char *buf, size_t count)
{
	unsigned int value;
	unsigned int ret;

	if (unlikely(kstrtoint(buf, 0, &value) != 1))
		return -EINVAL;

	if (value == 1) {
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

	if (value & 0x0000FFFF) {
		pr_debug("AXI violation.\n");
		pr_debug("[EMI MPU AXI] Debug info start ----------------------------------------\n");

		pr_debug("EMI_CHKER = %x.\n", value);
		pr_debug("Violation address is 0x%x.\n",
		readl(IOMEM(EMI_CHKER_ADR)));
		pr_debug("Violation master ID is 0x%x.\n", master_ID);
		pr_debug("Violation type is: AXI_ADR_CHK_EN(%d), AXI_LOCK_CHK_EN(%d), AXI_NON_ALIGN_CHK_EN(%d).\n",
			   (value & (1 << AXI_ADR_VIO)) ? 1 : 0,
			   (value & (1 << AXI_LOCK_ISSUE)) ? 1 : 0,
			   (value & (1 << AXI_NON_ALIGN_ISSUE)) ? 1 : 0);
		pr_debug("%s violation.\n",
		(value & (1 << AXI_VIO_WR)) ? "Write" : "Read");
		master_name = __id2name(master_ID);
		pr_debug("violation master_name is %s.\n", master_name);

		pr_debug("[EMI MPU AXI] Debug info end ----------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
		if (aee_ke_en)
			aee_kernel_exception("EMI MPU AXI",
			"AXI violation.\nEMI_CHKER = 0x%x\nCRDISPATCH_KEY:EMI MPU Violation Issue/%s\n",
			value, master_name);
#endif
		/* clear AXI checker status */
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
	/* assign timer to CPU0 to avoid CPU plug-out
	 * and timer will be unavailable
	 */

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

DRIVER_ATTR(emi_axi_vio,	0644, emi_axi_vio_show,	 emi_axi_vio_store);

#endif /* #ifdef ENABLE_EMI_CHKER */

#ifdef ENABLE_EMI_WATCH_POINT
static void emi_wp_set_address(unsigned int address)
{
	mt_reg_sync_writel(address - emi_physical_offset, EMI_WP_ADR);
}

static void emi_wp_set_range(unsigned int range)
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
	result |= readl(IOMEM(EMI_CHKER_ADR_2ND));

	if (result)
		pr_debug("[EMI_WP] Clear WP status fail!!!!!!!!!!!!!!\n");
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
	pr_debug("[EMI_WP] Violation master ID is 0x%x.\n", master_ID);
	pr_debug("[EMI_WP] Violation Address is : 0x%llx\n",
	(readl(IOMEM(EMI_CHKER_ADR)) + (((unsigned long long)(readl(IOMEM(EMI_CHKER_ADR_2ND)) & 0xF)) << 32)
	+ emi_physical_offset));

	master_name = __id2name(master_ID);
	pr_debug("[EMI_WP] EMI_CHKER = 0x%x, module is %s.\n",
	value, master_name);

	value = readl(IOMEM(EMI_CHKER_TYPE));
	pr_debug("[EMI_WP] Transaction Type is : %d beat, %d byte, %s burst type (0x%X)\n",
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
		pr_err("[EMI_WP] Range error, you can't set range less than 16 bytes and more than 4G bytes\n");
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
	unsigned int value, master_ID, type;
	unsigned long long vio_addr;
	char *master_name;
	char *ptr = buf;

	value = readl(IOMEM(EMI_CHKER));

	if ((value & 0x80000000) == 0) {
		return snprintf(buf, PAGE_SIZE,
		"[EMI_WP] No watch point hit\n");
	}

	master_ID = (value & EMI_WP_AXI_ID);
	master_name = __id2name(master_ID);

	type = readl(IOMEM(EMI_CHKER_TYPE));
	vio_addr = (readl(IOMEM(EMI_CHKER_ADR)) + (((unsigned long long)(readl(IOMEM(EMI_CHKER_ADR_2ND)) & 0xF)) << 32)
	+ emi_physical_offset);
	emi_wp_clr_status();
	ptr += snprintf(ptr, PAGE_SIZE, "[EMI WP] vio setting is: CHKER 0x%X, ",
			value);

	ptr += snprintf(ptr, PAGE_SIZE, "module is %s, Address is : 0x%llX,",
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
	pr_debug("emi_wp_store: %s\n", buf);

	command = kmalloc((size_t)MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strcpy(command, buf);
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
		pr_debug("Set EMI_WP: address: 0x%x, range:%d, start addr: 0x%x, end addr: 0x%x,  rw: %d .\n",
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
		pr_debug("disable EMI WP\n");
	} else {
		pr_err("Unknown emi_wp command.\n");
	}

out:
	kfree(command);

	return count;
}


DRIVER_ATTR(emi_wp_vio, 0644, emi_wp_vio_show, emi_wp_vio_store);
#endif /* #ifdef ENABLE_EMI_WATCH_POINT */

#if 0
#define AP_REGION_ID   23
static void protect_ap_region(void)
{

	unsigned int ap_mem_mpu_id, ap_mem_mpu_attr;
	unsigned long long kernel_base;
	phys_addr_t dram_size;

	kernel_base = emi_physical_offset;
	dram_size = get_max_DRAM_size();

	ap_mem_mpu_id = AP_REGION_ID;
	ap_mem_mpu_attr = SET_ACCESS_PERMISSON(FORBIDDEN,
	SEC_R_NSEC_RW, FORBIDDEN, NO_PROTECTION, FORBIDDEN,
	FORBIDDEN, FORBIDDEN, NO_PROTECTION);

	emi_mpu_set_region_protection(kernel_base,
	(kernel_base+dram_size-1), ap_mem_mpu_id, ap_mem_mpu_attr);

}
#endif

static struct platform_driver emi_mpu_ctrl = {
	.driver = {
		.name = "emi_mpu_ctrl",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
	},
	.id_table = NULL,
};

static int __init emi_mpu_mod_init(void)
{
	int ret;
	struct device_node *node;
	unsigned int mpu_irq = 0;

#if EMI_MPU_TEST
	unsigned int *ptr0_phyical;

	ptr0_phyical = (unsigned int *)__pa(mpu_test_buffer);
	pr_debug("[EMI_MPU]   ptr0_phyical : %p\n", ptr0_phyical);
	*((volatile unsigned int *)(mpu_test_buffer+0x10000)) = 0xdeaddead;
#endif

	pr_info("Initialize EMI MPU.\n");

	isetocunt = 0;

	/* DTS version */
	if (EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,emi");
		if (node) {
			EMI_BASE_ADDR = of_iomap(node, 0);
			mpu_irq = irq_of_parse_and_map(node, 0);
			pr_debug("get EMI_MPU irq = %d\n", mpu_irq);
			pr_debug("get EMI_BASE_ADDR @ %p\n", EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (CHN0_EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,chn_emi");
		if (node) {
			CHN0_EMI_BASE_ADDR = of_iomap(node, 0);
			pr_debug("get CHN0_EMI_BASE_ADDR @ %p\n", CHN0_EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

#if 0	/* Zion has no channel 1 EMI */
	if (CHN1_EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_ch1_top3");
		if (node) {
			CHN1_EMI_BASE_ADDR = of_iomap(node, 0);
			pr_debug("get CHN0_EMI_BASE_ADDR @ %p\n", CHN1_EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}
#endif

	if (DRAMC_CH0_TOP0_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_ch0_top0");
		if (node) {
			DRAMC_CH0_TOP0_BASE_ADDR = of_iomap(node, 0);
			pr_debug("get DRAMC_CH0_TOP0_BASE_ADDR @ %p\n", DRAMC_CH0_TOP0_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (DRAMC_CH1_TOP0_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,dramc_ch1_top0");
		if (node) {
			DRAMC_CH1_TOP0_BASE_ADDR = of_iomap(node, 0);
			pr_debug("get DRAMC_CH1_TOP0_BASE_ADDR @ %p\n", DRAMC_CH1_TOP0_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (INFRA_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
		if (node) {
			INFRA_BASE_ADDR = of_iomap(node, 0);
			pr_debug("get INFRA_BASE_ADDR @ %p\n", INFRA_BASE_ADDR);
		} else {
			pr_err("can't find compatible node INFRA_BASE_ADDR\n");
			return -1;
		}
	}

	if (INFRACFG_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
		if (node) {
			INFRACFG_BASE_ADDR = of_iomap(node, 0);
			pr_debug("get INFRACFG_BASE_ADDR @ %p\n",
			INFRACFG_BASE_ADDR);
		} else {
			pr_err("can't find compatible node INFRACFG_BASE_ADDR\n");
			return -1;
		}
	}

	spin_lock_init(&emi_mpu_lock);

	pr_debug("[EMI MPU] EMI_MPUS = 0x%x\n", readl(IOMEM(EMI_MPUS)));
	pr_debug("[EMI MPU] EMI_MPUT = 0x%x\n", readl(IOMEM(EMI_MPUT)));
	pr_debug("[EMI MPU] EMI_MPUT_2ND = 0x%x\n", readl(IOMEM(EMI_MPUT_2ND)));

	pr_debug("[EMI MPU] EMI_WP_ADR = 0x%x\n", readl(IOMEM(EMI_WP_ADR)));
	pr_debug("[EMI MPU] EMI_WP_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_WP_ADR_2ND)));
	pr_debug("[EMI MPU] EMI_WP_CTRL = 0x%x\n", readl(IOMEM(EMI_WP_CTRL)));
	pr_debug("[EMI MPU] EMI_CHKER = 0x%x\n", readl(IOMEM(EMI_CHKER)));
	pr_debug("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n",
	readl(IOMEM(EMI_CHKER_TYPE)));
	pr_debug("[EMI MPU] EMI_CHKER_ADR = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR)));
	pr_debug("[EMI MPU] EMI_CHKER_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR_2ND)));

	if (readl(IOMEM(EMI_MPUS))) {
		pr_debug("[EMI MPU] get MPU violation in driver init\n");
		mpu_check_violation();
	} else {
		__clear_emi_mpu_vio(0);
		/* Set Device APC initialization for EMI-MPU. */
	}

	if (enable_4gb)
		emi_physical_offset = 0;
	else
		emi_physical_offset = 0x40000000;

	/*
	 * NoteXXX: Interrupts of violation (including SPC in SMI, or EMI MPU)
	 *		  are triggered by the device APC.
	 *		  Need to share the interrupt with the SPC driver.
	 */
	if (mpu_irq != 0) {
		ret = request_irq(mpu_irq,
		(irq_handler_t)mpu_violation_irq, IRQF_TRIGGER_LOW | IRQF_SHARED,
		"mt_emi_mpu", &emi_mpu_ctrl);
		if (ret != 0) {
			pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
			return ret;
		}
	}

#if 0
	protect_ap_region();
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
#endif /* #ifdef ENABLE_EMI_CHKER */

#if !defined(USER_BUILD_KERNEL)

	/* register driver and create sysfs files */
	ret = platform_driver_register(&emi_mpu_ctrl);
	if (ret)
		pr_err("Fail to register EMI_MPU driver.\n");

	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_mpu_config);
	if (ret)
		pr_err("Fail to create MPU config sysfs file.\n");

#ifdef ENABLE_EMI_CHKER
ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_emi_axi_vio);

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

	/* Create a workqueue to search pagetable entry */
	emi_mpu_workqueue = create_singlethread_workqueue("emi_mpu");
	INIT_WORK(&emi_mpu_work, emi_mpu_work_callback);

	return 0;
}

static void __exit emi_mpu_mod_exit(void)
{
}

module_init(emi_mpu_mod_init);
module_exit(emi_mpu_mod_exit);

#ifdef CONFIG_MTK_LM_MODE
unsigned int enable_4G(void)
{
	return enable_4gb;
}

static int __init dram_4gb_init(void)
{
	void __iomem *INFRASYS_BASE_ADDR = NULL;
	struct device_node *node;

	/* get the information for 4G mode */
	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	if (node) {
		INFRASYS_BASE_ADDR = of_iomap(node, 0);
	} else {
		pr_err("INFRASYS_BASE_ADDR can't find compatible node\n");
		return -1;
	}

	if ((readl(IOMEM(INFRASYS_BASE_ADDR + 0xf00)) & 0x2000) == 0) {
		enable_4gb = 0;
		pr_debug("[EMI MPU] Not 4G mode\n");
	} else { /* enable 4G mode */
		enable_4gb = 1;
		pr_debug("[EMI MPU] 4G mode\n");
	}

	return 0;
}

early_initcall(dram_4gb_init);
#endif

#define INFRA_TOP_DBG_CON0           IOMEM(INFRACFG_BASE_ADDR+0x100)
#define INFRA_TOP_DBG_CON1           IOMEM(INFRACFG_BASE_ADDR+0x104)
#define INFRA_TOP_DBG_CON2           IOMEM(INFRACFG_BASE_ADDR+0x108)
#define INFRA_TOP_DBG_CON3           IOMEM(INFRACFG_BASE_ADDR+0x10C)

#define MISC_CG_PHY0_CTRL0            IOMEM(DRAMC_CH0_TOP0_BASE_ADDR+0x284)
#define MISC_CG_PHY0_CTRL2            IOMEM(DRAMC_CH0_TOP0_BASE_ADDR+0x28C)
#define MISC_CG_PHY1_CTRL0            IOMEM(DRAMC_CH1_TOP0_BASE_ADDR+0x284)

#define MEM_DCM_CTRL                 IOMEM(INFRA_BASE_ADDR+0x78)

#define EMI_CONI                     IOMEM(EMI_BASE_ADDR+0x040)
#define EMI_CONM                     IOMEM(EMI_BASE_ADDR+0x060)
#define EMI_CONN                     IOMEM(EMI_BASE_ADDR+0x068)
#define EMI_TEST0                    IOMEM(EMI_BASE_ADDR+0x0D0)
#define EMI_TEST1                    IOMEM(EMI_BASE_ADDR+0x0D8)
#define EMI_DFTB                     IOMEM(EMI_BASE_ADDR+0x0E8)
#define EMI_BMEN                     IOMEM(EMI_BASE_ADDR+0x400)
#define EMI_MSEL                     IOMEM(EMI_BASE_ADDR+0x440)
#define EMI_MSEL2                    IOMEM(EMI_BASE_ADDR+0x468)
#define EMI_BMEN2                    IOMEM(EMI_BASE_ADDR+0x4E8)
#define EMI_BMRW0                    IOMEM(EMI_BASE_ADDR+0x4F8)
#define EMI_WSCT                     IOMEM(EMI_BASE_ADDR+0x428)
#define EMI_WSCT2                    IOMEM(EMI_BASE_ADDR+0x458)
#define EMI_WSCT3                    IOMEM(EMI_BASE_ADDR+0x460)
#define EMI_WSCT4                    IOMEM(EMI_BASE_ADDR+0x464)
#define EMI_TTYPE1                   IOMEM(EMI_BASE_ADDR+0x500)
#define EMI_TTYPE9                   IOMEM(EMI_BASE_ADDR+0x540)
#define EMI_TTYPE2                   IOMEM(EMI_BASE_ADDR+0x508)
#define EMI_TTYPE10                  IOMEM(EMI_BASE_ADDR+0x548)
#define EMI_TTYPE3                   IOMEM(EMI_BASE_ADDR+0x510)
#define EMI_TTYPE11                  IOMEM(EMI_BASE_ADDR+0x550)
#define EMI_TTYPE4                   IOMEM(EMI_BASE_ADDR+0x518)
#define EMI_TTYPE12                  IOMEM(EMI_BASE_ADDR+0x558)
#define EMI_TTYPE5                   IOMEM(EMI_BASE_ADDR+0x520)
#define EMI_TTYPE13                  IOMEM(EMI_BASE_ADDR+0x560)
#define EMI_TTYPE6                   IOMEM(EMI_BASE_ADDR+0x528)
#define EMI_TTYPE14                  IOMEM(EMI_BASE_ADDR+0x568)
#define EMI_TTYPE7                   IOMEM(EMI_BASE_ADDR+0x530)
#define EMI_TTYPE15                  IOMEM(EMI_BASE_ADDR+0x570)
#define EMI_TTYPE8                   IOMEM(EMI_BASE_ADDR+0x538)
#define EMI_TTYPE16                  IOMEM(EMI_BASE_ADDR+0x578)
#define EMI_APBDBG                   IOMEM(EMI_BASE_ADDR+0x7FC)

#define CHN0_EMI_CONB                IOMEM(CHN0_EMI_BASE_ADDR+0x008)
#define CHN0_EMI_DBGA                IOMEM(CHN0_EMI_BASE_ADDR+0xA80)
#define CHN0_EMI_DBGB                IOMEM(CHN0_EMI_BASE_ADDR+0xA84)
#define CHN0_EMI_DBGC                IOMEM(CHN0_EMI_BASE_ADDR+0xA88)
#define CHN0_EMI_DBGD                IOMEM(CHN0_EMI_BASE_ADDR+0xA8C)

#if 0	/* Zion has no channel 1 EMI */
#define CHN1_EMI_CONB                IOMEM(CHN1_EMI_BASE_ADDR+0x008)
#define CHN1_EMI_DBGA                IOMEM(CHN1_EMI_BASE_ADDR+0xA80)
#define CHN1_EMI_DBGB                IOMEM(CHN1_EMI_BASE_ADDR+0xA84)
#define CHN1_EMI_DBGC                IOMEM(CHN1_EMI_BASE_ADDR+0xA88)
#define CHN1_EMI_DBGD                IOMEM(CHN1_EMI_BASE_ADDR+0xA8C)
#endif

void dump_emi_latency(void)
{
	/* Disable EMI DCM */
	pr_debug("Disable EMI DCM\n");
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~(1<<7)), MEM_DCM_CTRL);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (0x1f<<1), MEM_DCM_CTRL);
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | 0x1,       MEM_DCM_CTRL);
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~0x1),    MEM_DCM_CTRL);
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (1<<31),   MEM_DCM_CTRL);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (1<<28),   MEM_DCM_CTRL);	/* Disable DDRPHY DCM */

	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL0) | (1<<8),  MISC_CG_PHY0_CTRL0);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL0) | (1<<9),  MISC_CG_PHY0_CTRL0);	/* Disable DDRPHY DCM */
	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL2) | (1<<31), MISC_CG_PHY0_CTRL2);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL2) | (1<<28), MISC_CG_PHY0_CTRL2);	/* Disable DDRPHY DCM */

	/* Disable EMI clock gated */
	mt_reg_sync_writel(readl(EMI_CONM) | 0xff00f000, EMI_CONM);
	mt_reg_sync_writel(readl(EMI_CONN) | 0xff000000, EMI_CONN);

	mt_reg_sync_writel(readl(CHN0_EMI_CONB) | 0xff000000, CHN0_EMI_CONB);	/* Disable chn0_emi dcm */
#if 0	/* Zion has no channel 1 EMI */
	mt_reg_sync_writel(readl(CHN1_EMI_CONB) | 0xff000000, CHN1_EMI_CONB);	/* Disable chn1_emi dcm */
#endif

	/* clear bus monitor */
	mt_reg_sync_writel(readl(EMI_CONM) | (1 << 30), EMI_CONM);
	mt_reg_sync_writel(readl(EMI_BMEN) & 0xfffffffc, EMI_BMEN);	/* clear bus monitor */

	/* Enable EMI bus monitor */
	pr_debug("Enable EMI bus monitor\n");
	mt_reg_sync_writel(0x00FF0000, EMI_BMEN);	/* Reset counter and set WSCT  for ALL */
	mt_reg_sync_writel(0x00240003, EMI_MSEL);	/* Set WSCT3 for MM, WSCT2 for APMCU */
	mt_reg_sync_writel(0x00000018, EMI_MSEL2);	/* Set WSCT4 for MD */
	mt_reg_sync_writel(0x02000000, EMI_BMEN2);	/* Enable latency monitor */
	mt_reg_sync_writel(0x55555555, EMI_BMRW0);	/* Set latency monitor for read */
	mt_reg_sync_writel(readl(EMI_BMEN) | 0x1, EMI_BMEN);	/* Enable bus monitor */

	/* Wait for a while */
	mdelay(5);

	/* Get latency result */
	pr_debug("Get latency Result\n");
	mt_reg_sync_writel(readl(EMI_BMEN) | 0x2, EMI_BMEN);	/* Pause bus monitor */

	pr_debug("-----------Get BW RESULT----------------\n");
	/* Get BW result */
	pr_debug("EMI_WSCT = 0x%x (Total BW)\n", readl(EMI_WSCT));
	pr_debug("EMI_WSCT2 = 0x%x (APMCU(M0) BW)\n", readl(EMI_WSCT2));
	pr_debug("EMI_WSCT3 = 0x%x (MM(M5) BW)\n", readl(EMI_WSCT3));
	pr_debug("EMI_WSCT4 = 0x%x (MDMCU(M3) BW)\n", readl(EMI_WSCT4));

	/* Get latency result */
	/* APMCU(M0) read latency = EMI_TTYPE1 / EMI_TTYPE9 */
	pr_debug("EMI_TTYPE1 = 0x%x\n", readl(EMI_TTYPE1));
	pr_debug("EMI_TTYPE9 = 0x%x\n", readl(EMI_TTYPE9));
	/* MDMCU(M3) read latency = EMI_TTYPE4 / EMI_TTYPE12 */
	pr_debug("EMI_TTYPE4 = 0x%x\n", readl(EMI_TTYPE4));
	pr_debug("EMI_TTYPE12 = 0x%x\n", readl(EMI_TTYPE12));
	/* MDHW(M4)  read latency = EMI_TTYPE5 / EMI_TTYPE13 */
	pr_debug("EMI_TTYPE5 = 0x%x\n", readl(EMI_TTYPE5));
	pr_debug("EMI_TTYPE13 = 0x%x\n", readl(EMI_TTYPE13));
	/* MM(M5)    read latency = EMI_TTYPE6 / EMI_TTYPE14 */
	pr_debug("EMI_TTYPE6 = 0x%x\n", readl(EMI_TTYPE6));
	pr_debug("EMI_TTYPE14 = 0x%x\n", readl(EMI_TTYPE14));
	/* GPU(M6)   read latency = EMI_TTYPE7 / EMI_TTYPE15 */
	pr_debug("EMI_TTYPE7 = 0x%x\n", readl(EMI_TTYPE7));
	pr_debug("EMI_TTYPE15 = 0x%x\n", readl(EMI_TTYPE15));

	pr_debug("EMI_CONI  = 0x%x\n", readl(EMI_CONI));	/* GPU OSTD setting when LTE busy */
	pr_debug("EMI_CONM  = 0x%x\n", readl(EMI_CONM));	/* EMI DCM */
	pr_debug("EMI_TEST0 = 0x%x\n", readl(EMI_TEST0));	/* OSTD setting of M0~M3 */
	pr_debug("EMI_TEST1 = 0x%x\n", readl(EMI_TEST1));	/* OSTD settinf of M4~M5 */
	pr_debug("DDRPHY_0_CG = 0x%x\n", readl(MISC_CG_PHY0_CTRL0));	/* [8]: chn_emi CG */
	pr_debug("DDRPHY_1_CG = 0x%x\n", readl(MISC_CG_PHY1_CTRL0));	/* [8]: chn_emi CG */
	pr_debug("MEM_DCM_CTRL  = 0x%x\n", readl(MEM_DCM_CTRL));	/* infra CG enable */
}

void dump_emi_MM(void)
{
	/* Disable EMI DCM */
	pr_debug("Disable EMI DCM\n");
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~(1<<7)), MEM_DCM_CTRL);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (0x1f<<1), MEM_DCM_CTRL);
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | 0x1,       MEM_DCM_CTRL);
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~0x1),    MEM_DCM_CTRL);
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (1<<31),   MEM_DCM_CTRL);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (1<<28),   MEM_DCM_CTRL);	/* Disable DDRPHY DCM */

	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL0) | (1<<8),  MISC_CG_PHY0_CTRL0);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL0) | (1<<9),  MISC_CG_PHY0_CTRL0);	/* Disable DDRPHY DCM */
	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL2) | (1<<31), MISC_CG_PHY0_CTRL2);	/* Disable EMI DCM */
	mt_reg_sync_writel(readl(MISC_CG_PHY0_CTRL2) | (1<<28), MISC_CG_PHY0_CTRL2);	/* Disable DDRPHY DCM */

	/* Disable EMI clock gated */
	mt_reg_sync_writel(readl(EMI_CONM) | 0xff00f000, EMI_CONM);
	mt_reg_sync_writel(readl(EMI_CONN) | 0xff000000, EMI_CONN);

	mt_reg_sync_writel(readl(CHN0_EMI_CONB) | 0xff000000, CHN0_EMI_CONB);	/* Disable chn0_emi dcm */
#if 0	/* Zion has no channel 1 EMI */
	mt_reg_sync_writel(readl(CHN1_EMI_CONB) | 0xff000000, CHN1_EMI_CONB);	/* Disable chn1_emi dcm */
#endif

	/* Enable EMI debug out */
	mt_reg_sync_writel(readl(EMI_DFTB) | (1<<8), EMI_DFTB);

	/* Setup EMI debug out */
	mt_reg_sync_writel(0x00000020, INFRA_TOP_DBG_CON0);		/* arvalid */
	mt_reg_sync_writel(0x00000040, INFRA_TOP_DBG_CON1);		/* arready */
	mt_reg_sync_writel(0x00000060, INFRA_TOP_DBG_CON2);		/* awvalid */
	mt_reg_sync_writel(0x00000080, INFRA_TOP_DBG_CON3);		/* awready */

	/* Read EMI debug out */
	pr_debug("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));

	/* Setup EMI debug out */
	mt_reg_sync_writel(0x00000100, INFRA_TOP_DBG_CON0);		/* rvalid */
	mt_reg_sync_writel(0x00000120, INFRA_TOP_DBG_CON1);		/* rready */
	mt_reg_sync_writel(0x00000200, INFRA_TOP_DBG_CON2);		/* axi_r_req */
	mt_reg_sync_writel(0x00000300, INFRA_TOP_DBG_CON3);		/* axi_r_gnt */

	/* Read EMI debug out */
	pr_debug("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));

	/* Setup EMI debug out */
	mt_reg_sync_writel(0x000002A0, INFRA_TOP_DBG_CON0);		/* rff_used[7:0]  */
	mt_reg_sync_writel(0x000002C0, INFRA_TOP_DBG_CON1);		/* rff_used[15:8] */
	mt_reg_sync_writel(0x000002E0, INFRA_TOP_DBG_CON2);		/* rdf_used[7:0]  */
	mt_reg_sync_writel(0x00000300, INFRA_TOP_DBG_CON3);		/* rdf_used[15:8] */

	/* Read EMI debug out */
	pr_debug("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));

	/* Setup EMI debug out */
	mt_reg_sync_writel(0x00000320, INFRA_TOP_DBG_CON0);		/* wff_used[7:0]  */
	mt_reg_sync_writel(0x00000340, INFRA_TOP_DBG_CON1);		/* wff_used[15:8] */
	mt_reg_sync_writel(0x00000360, INFRA_TOP_DBG_CON2);		/* 6'b0, wff_used[17:16] */
	mt_reg_sync_writel(0x00000380, INFRA_TOP_DBG_CON3);		/* rdf_used[19:16], rff_used[19:16] */

	/* Read EMI debug out */
	pr_debug("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));

	/* clear bus monitor */
	mt_reg_sync_writel(readl(EMI_CONM) | (1 << 30), EMI_CONM);
	mt_reg_sync_writel(readl(EMI_BMEN) & 0xfffffffc, EMI_BMEN);	/* clear bus monitor */

	/* Setup EMI bus monitor */
	pr_debug("Setup EMI bus monitor\n");
	mt_reg_sync_writel(0x00FF0000, EMI_BMEN);	/* Reset counter and set WSCT for ALL */
	mt_reg_sync_writel(0x00240003, EMI_MSEL);	/* Set WSCT3 for MM, WSCT2 for APMCU */
	mt_reg_sync_writel(0x00000018, EMI_MSEL2);	/* Set WSCT4 for MD */
	mt_reg_sync_writel(0x02000000, EMI_BMEN2);	/* Enable latency monitor */
	mt_reg_sync_writel(0x55555555, EMI_BMRW0);	/* Set latency monitor for read */
	mt_reg_sync_writel(readl(EMI_BMEN) | 0x1, EMI_BMEN);	/* Enable bus monitor */

	/* Wait for a while */
	mdelay(5);

	/* Get latency result */
	mt_reg_sync_writel(readl(EMI_BMEN) | 0x2, EMI_BMEN);	/* Pause bus monitor */

	/* Get BW result */
	pr_debug("EMI_WSCT = 0x%x (Total BW)\n", readl(EMI_WSCT));
	pr_debug("EMI_WSCT2 = 0x%x (APMCU(M0) BW)\n", readl(EMI_WSCT2));
	pr_debug("EMI_WSCT3 = 0x%x (MM(M5) BW)\n", readl(EMI_WSCT3));
	pr_debug("EMI_WSCT4 = 0x%x (MDMCU(M3) BW)\n", readl(EMI_WSCT4));

	/* Get latency result and check pending commands */
	pr_debug("EMI_TTYPE1 = 0x%x\n", readl(EMI_TTYPE1));   /* APMCU0(M0) total read latency count */
	pr_debug("EMI_TTYPE9 = 0x%x\n", readl(EMI_TTYPE9));   /* APMCU0(M0) total read trans count   */
	pr_debug("EMI_TTYPE2 = 0x%x\n", readl(EMI_TTYPE2));   /* APMCU1(M1) total read latency count */
	pr_debug("EMI_TTYPE10 = 0x%x\n", readl(EMI_TTYPE10)); /* APMCU1(M1) total read trans count   */
	pr_debug("EMI_TTYPE3 = 0x%x\n", readl(EMI_TTYPE3));   /* MM1(M2)    total read latency count */
	pr_debug("EMI_TTYPE11 = 0x%x\n", readl(EMI_TTYPE11)); /* MM1(M2)    total read trans count   */
	pr_debug("EMI_TTYPE4 = 0x%x\n", readl(EMI_TTYPE4));   /* MDMCU(M3)  total read latency count */
	pr_debug("EMI_TTYPE12 = 0x%x\n", readl(EMI_TTYPE12)); /* MDMCU(M3)  total read trans count   */
	pr_debug("EMI_TTYPE5 = 0x%x\n", readl(EMI_TTYPE5));   /* MDHW(M4)   total read latency count */
	pr_debug("EMI_TTYPE13 = 0x%x\n", readl(EMI_TTYPE13)); /* MDHW(M4)   total read trans count   */
	pr_debug("EMI_TTYPE6 = 0x%x\n", readl(EMI_TTYPE6));   /* MM0(M5)    total read latency count */
	pr_debug("EMI_TTYPE14 = 0x%x\n", readl(EMI_TTYPE14)); /* MM0(M5)    total read trans count   */
	pr_debug("EMI_TTYPE7 = 0x%x\n", readl(EMI_TTYPE7));   /* PERI(M6)   total read latency count */
	pr_debug("EMI_TTYPE15 = 0x%x\n", readl(EMI_TTYPE15)); /* PERI(M6)   total read trans count   */
	pr_debug("EMI_TTYPE8 = 0x%x\n", readl(EMI_TTYPE8));   /* GPU(M7)    total read latency count */
	pr_debug("EMI_TTYPE16 = 0x%x\n", readl(EMI_TTYPE16)); /* GPU(M7)    total read trans count   */

	/* clear bus monitor */
	mt_reg_sync_writel(readl(EMI_CONM) | (1 << 30), EMI_CONM);
	mt_reg_sync_writel(readl(EMI_BMEN) & 0xfffffffc, EMI_BMEN);	/* clear bus monitor */

	/* Setup EMI bus monitor */
	pr_debug("Setup EMI bus monitor\n");
	mt_reg_sync_writel(0x00FF0000, EMI_BMEN);	/* Reset counter and set WSCT for ALL */
	mt_reg_sync_writel(0x00240003, EMI_MSEL);	/* Set WSCT3 for MM, WSCT2 for APMCU */
	mt_reg_sync_writel(0x00000018, EMI_MSEL2);	/* Set WSCT4 for MD */
	mt_reg_sync_writel(0x02000000, EMI_BMEN2);	/* Enable latency monitor */
	mt_reg_sync_writel(0xAAAAAAAA, EMI_BMRW0);	/* Set latency monitor for write */
	mt_reg_sync_writel(readl(EMI_BMEN) | 0x1, EMI_BMEN);	/* Enable bus monitor */

	/* Wait for a while */
	mdelay(5);

	/* Get latency result */
	mt_reg_sync_writel(readl(EMI_BMEN) | 0x2, EMI_BMEN);	/* Pause bus monitor */

	/* Get BW result */
	pr_debug("EMI_WSCT = 0x%x (Total BW)\n", readl(EMI_WSCT));
	pr_debug("EMI_WSCT2 = 0x%x (APMCU(M0) BW)\n", readl(EMI_WSCT2));
	pr_debug("EMI_WSCT3 = 0x%x (MM(M5) BW)\n", readl(EMI_WSCT3));
	pr_debug("EMI_WSCT4 = 0x%x (MDMCU(M3) BW)\n", readl(EMI_WSCT4));

	/* Get latency result and check pending commands */
	pr_debug("EMI_TTYPE1 = 0x%x\n", readl(EMI_TTYPE1));   /* APMCU0(M0) total write latency count */
	pr_debug("EMI_TTYPE9 = 0x%x\n", readl(EMI_TTYPE9));   /* APMCU0(M0) total write trans count   */
	pr_debug("EMI_TTYPE2 = 0x%x\n", readl(EMI_TTYPE2));   /* APMCU1(M1) total write latency count */
	pr_debug("EMI_TTYPE10 = 0x%x\n", readl(EMI_TTYPE10)); /* APMCU1(M1) total write trans count   */
	pr_debug("EMI_TTYPE3 = 0x%x\n", readl(EMI_TTYPE3));   /* MM1(M2)    total write latency count */
	pr_debug("EMI_TTYPE11 = 0x%x\n", readl(EMI_TTYPE11)); /* MM1(M2)    total write trans count   */
	pr_debug("EMI_TTYPE4 = 0x%x\n", readl(EMI_TTYPE4));   /* MDMCU(M3)  total write latency count */
	pr_debug("EMI_TTYPE12 = 0x%x\n", readl(EMI_TTYPE12)); /* MDMCU(M3)  total write trans count   */
	pr_debug("EMI_TTYPE5 = 0x%x\n", readl(EMI_TTYPE5));   /* MDHW(M4)   total write latency count */
	pr_debug("EMI_TTYPE13 = 0x%x\n", readl(EMI_TTYPE13)); /* MDHW(M4)   total write trans count   */
	pr_debug("EMI_TTYPE6 = 0x%x\n", readl(EMI_TTYPE6));   /* MM0(M5)    total write latency count */
	pr_debug("EMI_TTYPE14 = 0x%x\n", readl(EMI_TTYPE14)); /* MM0(M5)    total write trans count   */
	pr_debug("EMI_TTYPE7 = 0x%x\n", readl(EMI_TTYPE7));   /* PERI(M6)   total write latency count */
	pr_debug("EMI_TTYPE15 = 0x%x\n", readl(EMI_TTYPE15)); /* PERI(M6)   total write trans count   */
	pr_debug("EMI_TTYPE8 = 0x%x\n", readl(EMI_TTYPE8));   /* GPU(M7)    total write latency count */
	pr_debug("EMI_TTYPE16 = 0x%x\n", readl(EMI_TTYPE16)); /* GPU(M7)    total write trans count   */

	/* channel EMI */
	/* channel 0 */
	mt_reg_sync_writel(readl(CHN0_EMI_DBGA) | 0x1, CHN0_EMI_DBGA);
	mt_reg_sync_writel(0x160015, CHN0_EMI_DBGC);
	mt_reg_sync_writel(0x180017, CHN0_EMI_DBGD);
	pr_debug("CHN0_EMI_DBGB = 0x%x (rdf_used[15:0], rff_used[15:0])\n", readl(CHN0_EMI_DBGB));

	mt_reg_sync_writel(0x1a0019, CHN0_EMI_DBGC);
	mt_reg_sync_writel(0x24001b, CHN0_EMI_DBGD);
	pr_debug("CHN0_EMI_DBGB = 0x%x (wff_ch0_req[7:0], 6'b0 ,wff_used[17:0])\n", readl(CHN0_EMI_DBGB));

	mt_reg_sync_writel(0x370036, CHN0_EMI_DBGC);
	mt_reg_sync_writel(0x390038, CHN0_EMI_DBGD);
	pr_debug("CHN0_EMI_DBGB = 0x%x (rff_remain[7:0], rdf_remain[17:0])\n", readl(CHN0_EMI_DBGB));

	mt_reg_sync_writel(0x050004, CHN0_EMI_DBGC);
	mt_reg_sync_writel(0x070006, CHN0_EMI_DBGD);
	pr_debug("CHN0_EMI_DBGB = 0x%x (seq0_len_cnt, {4'b0, seq0_fifo_cnt}, seq0_wid, seq0_rid)\n",
		readl(CHN0_EMI_DBGB));

	mt_reg_sync_writel(0x0e0008, CHN0_EMI_DBGC);
	mt_reg_sync_writel(0x10000f, CHN0_EMI_DBGD);
	pr_debug("CHN0_EMI_DBGB = 0x%x\n", readl(CHN0_EMI_DBGB));
	pr_debug("(seq0_wdata_cnt[8], seq0_rdata_cnt[8], chn0_adr[13:8])\n");
	pr_debug("(chn_sbr_push, 1'b0, chn0_llat, chn0_hpri, chn0_rank, chn0_wr, chn0_new_req, chn0_req, chn0_cdwf)\n");
	pr_debug("(2'b0, seq0_len_pop, seq0_rdy, seq0_rlast, seq0_64b_dle, seq0_dle, seq0_wdle)\n");

	mt_reg_sync_writel(0x0a0009, CHN0_EMI_DBGC);
	mt_reg_sync_writel(0x0c000b, CHN0_EMI_DBGD);
	pr_debug("CHN0_EMI_DBGB = 0x%x (seq0_bank_mask[15:0], seq0_wdata_cnt[7:0], seq0_rdata_cnt[7:0])\n",
		readl(CHN0_EMI_DBGB));

#if 0	/* Zion has no channel 1 EMI */
	/* channel 1 */
	mt_reg_sync_writel(readl(CHN1_EMI_DBGA) | 0x1, CHN1_EMI_DBGA);
	mt_reg_sync_writel(0x160015, CHN1_EMI_DBGC);
	mt_reg_sync_writel(0x180017, CHN1_EMI_DBGD);
	pr_debug("CHN1_EMI_DBGB = 0x%x (rdf_used[15:0], rff_used[15:0])\n", readl(CHN1_EMI_DBGB));

	mt_reg_sync_writel(0x1a0019, CHN1_EMI_DBGC);
	mt_reg_sync_writel(0x24001b, CHN1_EMI_DBGD);
	pr_debug("CHN1_EMI_DBGB = 0x%x (wff_ch0_req[7:0], 6'b0 ,wff_used[17:0])\n", readl(CHN1_EMI_DBGB));

	mt_reg_sync_writel(0x370036, CHN1_EMI_DBGC);
	mt_reg_sync_writel(0x390038, CHN1_EMI_DBGD);
	pr_debug("CHN1_EMI_DBGB = 0x%x (rff_remain[7:0], rdf_remain[17:0])\n", readl(CHN1_EMI_DBGB));

	mt_reg_sync_writel(0x050004, CHN1_EMI_DBGC);
	mt_reg_sync_writel(0x070006, CHN1_EMI_DBGD);
	pr_debug("CHN1_EMI_DBGB = 0x%x (seq0_len_cnt, {4'b0, seq0_fifo_cnt}, seq0_wid, seq0_rid)\n",
		readl(CHN1_EMI_DBGB));

	mt_reg_sync_writel(0x0e0008, CHN1_EMI_DBGC);
	mt_reg_sync_writel(0x10000f, CHN1_EMI_DBGD);
	pr_debug("CHN1_EMI_DBGB = 0x%x\n", readl(CHN1_EMI_DBGB));
	pr_debug("(seq0_wdata_cnt[8], seq0_rdata_cnt[8], chn0_adr[13:8])\n");
	pr_debug("(chn_sbr_push, 1'b0, chn0_llat, chn0_hpri, chn0_rank, chn0_wr, chn0_new_req, chn0_req, chn0_cdwf)\n");
	pr_debug("(2'b0, seq0_len_pop, seq0_rdy, seq0_rlast, seq0_64b_dle, seq0_dle, seq0_wdle)\n");

	mt_reg_sync_writel(0x0a0009, CHN1_EMI_DBGC);
	mt_reg_sync_writel(0x0c000b, CHN1_EMI_DBGD);
	pr_debug("CHN1_EMI_DBGB = 0x%x (seq0_bank_mask[15:0], seq0_wdata_cnt[7:0], seq0_rdata_cnt[7:0])\n",
		readl(CHN1_EMI_DBGB));
#endif
}
