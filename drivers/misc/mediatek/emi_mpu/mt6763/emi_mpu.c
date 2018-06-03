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
#include <mt-plat/mtk_secure_api.h>
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
#include "emi_mpu.h"
#include <mt-plat/mtk_ccci_common.h>
#include <linux/delay.h>

static int Violation_PortID = MASTER_ALL;

#define EMI_MPU_TEST	0
#if EMI_MPU_TEST
char mpu_test_buffer[0x20000] __aligned(PAGE_SIZE);
#endif

#define ENABLE_EMI_CHKER
#define ENABLE_EMI_WATCH_POINT

#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define AXI_VIO_MONITOR_TIME	(1 * HZ)


static void __iomem *CEN_EMI_BASE;
static void __iomem *CHA_EMI_BASE;
static void __iomem *CHB_EMI_BASE;
static void __iomem *EMI_MPU_BASE;
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

static const struct mst_tbl_entry mst_tbl[] = {
	/* APMCU 1 */
	{ .master = MST_ID_APMCU_CH1_0,  .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x4,
		.note = "CA7LL: Core nn system domain store exclusive",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_1,  .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x44,
		.note = "CA7LL: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_2,  .port = 0x0, .id_mask = 0xFFF,
		.id_val = 0x94,
		.note = "CA7LL: SCU generated barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_3,  .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0xC4,
		.note = "CA7LL: Core nn non-re-orderable device write",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_4,  .port = 0x0, .id_mask = 0xF0F,
		.id_val = 0x104,
		.note = "CA7LL: Write to normal memory",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_5,  .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x4,
		.note = "CA7LL: Core nn exclusive read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_6,  .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x44,
		.note = "CA7LL: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_7,  .port = 0x0, .id_mask = 0xFFF,
		.id_val = 0x94,
		.note = "CA7LL: SCU generated barrier or DVM complete",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_8,  .port = 0x0, .id_mask = 0xF3F,
		.id_val = 0x104,
		.note = "CA7LL: ACP read",
		.name = "CA53_m_57"},
	{ .master = MST_ID_APMCU_CH1_9,  .port = 0x0, .id_mask = 0xE0F,
		.id_val = 0x204,
		.note = "CA7LL: Core nn read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_10, .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x3,
		.note = "CA7L: Core nn system domain store exclusive",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_11, .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x43,
		.note = "CA7L: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_12, .port = 0x0, .id_mask = 0xFFF,
		.id_val = 0x93,
		.note = "CA7L: SCU generated barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_13, .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0xC3,
		.note = "CA7L: Core nn non-re-orderable device write",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_14, .port = 0x0, .id_mask = 0xF0F,
		.id_val = 0x103,
		.note = "CA7L: Write to normal memory",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_15, .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x3,
		.note = "CA7L: Core nn exclusive read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_16, .port = 0x0, .id_mask = 0xFCF,
		.id_val = 0x43,
		.note = "CA7L: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_17, .port = 0x0, .id_mask = 0xFFF,
		.id_val = 0x93,
		.note = "CA7L: SCU generated barrier or DVM complete",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_18, .port = 0x0, .id_mask = 0xF3F,
		.id_val = 0x103,
		.note = "CA7L: ACP read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH1_19, .port = 0x0, .id_mask = 0xE0F,
		.id_val = 0x203,
		.note = "CA7L: Core nn read",
		.name = "CA53_m_57" },

	/* APMCU 2 */
	{ .master = MST_ID_APMCU_CH2_0,  .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x4,
		.note = "CA7LL: Core nn system domain store exclusive",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_1,  .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x44,
		.note = "CA7LL: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_2,  .port = 0x1, .id_mask = 0xFFF,
		.id_val = 0x94,
		.note = "CA7LL: SCU generated barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_3,  .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0xC4,
		.note = "CA7LL: Core nn non-re-orderable device write",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_4,  .port = 0x1, .id_mask = 0xF0F,
		.id_val = 0x104,
		.note = "CA7LL: Write to normal memory",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_5,  .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x4,
		.note = "CA7LL: Core nn exclusive read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_6,  .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x44,
		.note = "CA7LL: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_7,  .port = 0x1, .id_mask = 0xFFF,
		.id_val = 0x94,
		.note = "CA7LL: SCU generated barrier or DVM complete",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_8,  .port = 0x1, .id_mask = 0xF3F,
		.id_val = 0x104,
		.note = "CA7LL: ACP read",
		.name = "CA53_m_57"},
	{ .master = MST_ID_APMCU_CH2_9,  .port = 0x1, .id_mask = 0xE0F,
		.id_val = 0x204,
		.note = "CA7LL: Core nn read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_10, .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x3,
		.note = "CA7L: Core nn system domain store exclusive",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_11, .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x43,
		.note = "CA7L: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_12, .port = 0x1, .id_mask = 0xFFF,
		.id_val = 0x93,
		.note = "CA7L: SCU generated barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_13, .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0xC3,
		.note = "CA7L: Core nn non-re-orderable device write",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_14, .port = 0x1, .id_mask = 0xF0F,
		.id_val = 0x103,
		.note = "CA7L: Write to normal memory",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_15, .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x3,
		.note = "CA7L: Core nn exclusive read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_16, .port = 0x1, .id_mask = 0xFCF,
		.id_val = 0x43,
		.note = "CA7L: Core nn barrier",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_17, .port = 0x1, .id_mask = 0xFFF,
		.id_val = 0x93,
		.note = "CA7L: SCU generated barrier or DVM complete",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_18, .port = 0x1, .id_mask = 0xF3F,
		.id_val = 0x103,
		.note = "CA7L: ACP read",
		.name = "CA53_m_57" },
	{ .master = MST_ID_APMCU_CH2_19, .port = 0x1, .id_mask = 0xE0F,
		.id_val = 0x203,
		.note = "CA7L: Core nn read",
		.name = "CA53_m_57" },

	/* MM 1 */
	{ .master = MST_ID_MM_CH1_0, .port = 0x2, .id_mask = 0xF80,
		.id_val = 0x0, .note = "smi_larb0",
		.name = "smi_larb0_m_57" },
	{ .master = MST_ID_MM_CH1_1, .port = 0x2, .id_mask = 0xF80,
		.id_val = 0x80, .note = "smi_larb1",
		.name = "smi_larb1_m_57" },
	{ .master = MST_ID_MM_CH1_2, .port = 0x2, .id_mask = 0xF80,
		.id_val = 0x100, .note = "smi_larb2",
		.name = "smi_larb2_m_57" },
	{ .master = MST_ID_MM_CH1_3, .port = 0x2, .id_mask = 0xF80,
		.id_val = 0x180, .note = "smi_larb3",
		.name = "smi_larb3_m_57" },
	{ .master = MST_ID_MM_CH1_4, .port = 0x2, .id_mask = 0xF80,
		.id_val = 0x200, .note = "smi_larb4",
		.name = "smi_larb4_m_57" },
	{ .master = MST_ID_MM_CH1_5, .port = 0x2, .id_mask = 0xF80,
		.id_val = 0x280, .note = "smi_larb5",
		.name = "smi_larb5_m_57" },
	{ .master = MST_ID_MM_CH1_6, .port = 0x2, .id_mask = 0xFFF,
		.id_val = 0x3FC, .note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU_Internal_Used_m_57" },
	{ .master = MST_ID_MM_CH1_7, .port = 0x2, .id_mask = 0xFFF,
		.id_val = 0x3FD, .note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU_Internal_Used_m_57" },

	/* MDMCU*/
	{ .master = MST_ID_MDMCU_0,    .port = 0x3, .id_mask = 0xF0F,
		.id_val = 0x0, .note = "MDMCU",
		.name = "MDMCU_m_57" },
	{ .master = MST_ID_MDMCU_1,    .port = 0x3, .id_mask = 0xF0F,
		.id_val = 0x4, .note = "MDMCU PREFETCH",
		.name = "MDMCU_m_57" },
	{ .master = MST_ID_MDMCU_2,    .port = 0x3, .id_mask = 0xF0F,
		.id_val = 0x8, .note = "MDMCU ALC",
		.name = "MDMCU_m_57" },
	{ .master = MST_ID_MD_L1MCU_0, .port = 0x3, .id_mask = 0xF0F,
		.id_val = 0x1, .note = "L1MCU",
		.name = "L1MCU_m_57" },
	{ .master = MST_ID_MD_L1MCU_1, .port = 0x3, .id_mask = 0xF0F,
		.id_val = 0x5, .note = "L1MCU PREFETCH",
		.name = "L1MCU_m_57" },
	{ .master = MST_ID_MD_L1MCU_2, .port = 0x3, .id_mask = 0xF0F,
		.id_val = 0x9, .note = "L1MCU ALC",
		.name = "L1MCU_m_57" },
	{ .master = MST_ID_C2KMCU_0,   .port = 0x3, .id_mask = 0x3,
		.id_val = 0x2, .note = "C2KMCU ARM D port",
		.name = "C2KMCU_m_57" },

	/* MDDMA */
	{ .master = MST_ID_MDHW_0,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x0, .note = "AP2MDREG",
		.name = "MDHWMIX_AP2MDREG_m_57" },
	{ .master = MST_ID_MDHW_1,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x1, .note = "MDDBGSYS",
		.name = "MDHWMIX_MDDBGSYS_m_57" },
	{ .master = MST_ID_MDHW_2,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x2, .note = "PSMCU2REG",
		.name = "MDHWMIX_PSMCU2REG_m_57" },
	{ .master = MST_ID_MDHW_3,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x100, .note = "MDGDMA",
		.name = "MDHWMIX_MDGDMA_m_57" },
	{ .master = MST_ID_MDHW_4,  .port = 0x4, .id_mask = 0xFFE,
		.id_val = 0x200, .note = "HSPAL2",
		.name = "MDHWMIX_HSPAL2_m_57" },
	{ .master = MST_ID_MDHW_5,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x3C0, .note = "Rake_MD2G",
		.name = "MDHWMIX_Rake_MD2G_m_57" },
	{ .master = MST_ID_MDHW_6,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x340, .note = "DFE_B4",
		.name = "MDHWMIX_DFE_m_57" },
	{ .master = MST_ID_MDHW_7,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x341, .note = "DFE_DBG",
		.name = "MDHWMIX_DFE_m_57" },
	{ .master = MST_ID_MDHW_8,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x342, .note = "DFE_DBG1",
		.name = "MDHWMIX_DFE_m_57" },
	{ .master = MST_ID_MDHW_9,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x343, .note = "DFE_SHM",
		.name = "MDHWMIX_DFE_m_57" },
	{ .master = MST_ID_MDHW_10, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x380, .note = "RXBRP_HRQ_WR",
		.name = "MDHWMIX_RXBRP_m_57" },
	{ .master = MST_ID_MDHW_11, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x384, .note = "RXBRP_HRQ_WR1",
		.name = "MDHWMIX_RXBRP_m_57" },
	{ .master = MST_ID_MDHW_12, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x381, .note = "RXBRP_HRQ_RD",
		.name = "MDHWMIX_RXBRP_m_57" },
	{ .master = MST_ID_MDHW_13, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x385, .note = "RXBRP_HRQ_RD1",
		.name = "MDHWMIX_RXBRP_m_57" },
	{ .master = MST_ID_MDHW_14, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x382, .note = "RXBRP_HRQ_DBG",
		.name = "MDHWMIX_RXBRP_m_57" },
	{ .master = MST_ID_MDHW_15, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x386, .note = "RXBRP_HRQ_OTH",
		.name = "MDHWMIX_RXBRP_m_57" },
	{ .master = MST_ID_MDHW_16, .port = 0x4, .id_mask = 0xFC7,
		.id_val = 0x300, .note = "L1MCU",
		.name = "MDHWMIX_L1MCU_m_57" },
	{ .master = MST_ID_MDHW_17, .port = 0x4, .id_mask = 0xFFB,
		.id_val = 0x301, .note = "MDL1_GDMA",
		.name = "MDHWMIX_MDL1_GDMA_m_57" },
	{ .master = MST_ID_MDHW_18, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x302, .note = "MDL1_ABM",
		.name = "MDHWMIX_MDL1_ABM_m_57" },
	{ .master = MST_ID_MDHW_19, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x400, .note = "ARM7",
		.name = "MDHWMIX_ARM7_m_57" },
	{ .master = MST_ID_MDHW_20, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x500, .note = "PS_PERI",
		.name = "MDHWMIX_PS_PERI_m_57" },
	{ .master = MST_ID_MDHW_21, .port = 0x4, .id_mask = 0xFFE,
		.id_val = 0x600, .note = "LTEL2DMA",
		.name = "MDHWMIX_LTEL2DMA_m_57" },
	{ .master = MST_ID_MDHW_22, .port = 0x4, .id_mask = 0xFFE,
		.id_val = 0x700, .note = "SOE_TRACE",
		.name = "MDHWMIX_SOE_TRACE_m_57" },

	{ .master = MST_ID_LTE_0,  .port = 0x4, .id_mask = 0xFFB,
		.id_val = 0x800, .note = "CMP_DSPLOG",
		.name = "MDHWLTE_CMP_DSPLOG_m_57" },
	{ .master = MST_ID_LTE_1,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x806, .note = "CMP_DSPBTDMA",
		.name = "MDHWLTE_CMP_DSPBTDMA_m_57" },
	{ .master = MST_ID_LTE_2,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x802, .note = "CMP_CNWDMA",
		.name = "MDHWLTE_CMP_CNWDMA_m_57" },
	{ .master = MST_ID_LTE_3,  .port = 0x4, .id_mask = 0xFFD,
		.id_val = 0x810, .note = "CS",
		.name = "MDHWLTE_CS_m_57" },
	{ .master = MST_ID_LTE_4,  .port = 0x4, .id_mask = 0xFFB,
		.id_val = 0x820, .note = "IMC_RXTDB",
		.name = "MDHWLTE_IMC_RXTDB_m_57" },
	{ .master = MST_ID_LTE_5,  .port = 0x4, .id_mask = 0xFF7,
		.id_val = 0x822, .note = "IMC_DSPLOG",
		.name = "MDHWLTE_IMC_DSPLOG_m_57" },
	{ .master = MST_ID_LTE_6,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x82E, .note = "IMC_DSPBTDMA",
		.name = "MDHWLTE_IMC_DSPBTDMA_m_57" },
	{ .master = MST_ID_LTE_7,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x826, .note = "IMC_CNWDMA",
		.name = "MDHWLTE_IMC_CNWDMA_m_57" },
	{ .master = MST_ID_LTE_8,  .port = 0x4, .id_mask = 0xFFB,
		.id_val = 0x830, .note = "ICC_DSPLOG",
		.name = "MDHWLTE_ICC_DSPLOG_m_57" },
	{ .master = MST_ID_LTE_9,  .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x836, .note = "ICC_DSPBTDMA",
		.name = "MDHWLTE_ICC_DSPBTDMA_m_57" },
	{ .master = MST_ID_LTE_10, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x832, .note = "ICC_CNWDMA",
		.name = "MDHWLTE_ICC_CNWDMA_m_57" },
	{ .master = MST_ID_LTE_11, .port = 0x4, .id_mask = 0xFFD,
		.id_val = 0x840, .note = "RXDMP",
		.name = "MDHWLTE_RXDMP_m_57" },
	{ .master = MST_ID_LTE_12, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x801, .note = "cs id0",
		.name = "MDHWLTE_CS_m_57" },
	{ .master = MST_ID_LTE_13, .port = 0x4, .id_mask = 0xFFF,
		.id_val = 0x803, .note = "cs id1",
		.name = "MDHWLTE_CS_m_57" },

	/* MM 2 */
	{ .master = MST_ID_MM_CH2_0, .port = 0x5, .id_mask = 0xF80,
		.id_val = 0x0, .note = "smi_larb0",
		.name = "smi_larb0_m_57" },
	{ .master = MST_ID_MM_CH2_1, .port = 0x5, .id_mask = 0xF80,
		.id_val = 0x80, .note = "smi_larb1",
		.name = "smi_larb1_m_57" },
	{ .master = MST_ID_MM_CH2_2, .port = 0x5, .id_mask = 0xF80,
		.id_val = 0x100, .note = "smi_larb2",
		.name = "smi_larb2_m_57" },
	{ .master = MST_ID_MM_CH2_3, .port = 0x5, .id_mask = 0xF80,
		.id_val = 0x180, .note = "smi_larb3",
		.name = "smi_larb3_m_57" },
	{ .master = MST_ID_MM_CH2_4, .port = 0x5, .id_mask = 0xF80,
		.id_val = 0x200, .note = "smi_larb4",
		.name = "smi_larb4_m_57" },
	{ .master = MST_ID_MM_CH2_5, .port = 0x5, .id_mask = 0xF80,
		.id_val = 0x280, .note = "smi_larb5",
		.name = "smi_larb5_m_57" },
	{ .master = MST_ID_MM_CH2_6, .port = 0x5, .id_mask = 0xFFF,
		.id_val = 0x3FC, .note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU_Internal_Used_m_57" },
	{ .master = MST_ID_MM_CH2_7, .port = 0x5, .id_mask = 0xFFF,
		.id_val = 0x3FD, .note = "MM IOMMU Internal Used",
		.name = "MM_IOMMU_Internal_Used_m_57" },

	/* MFG */
	{ .master = MST_ID_MFG_0, .port = 0x6, .id_mask = 0xFC0,
		.id_val = 0x0, .note = "MFG", .name = "MFG_m_57" },
	{ .master = MST_ID_MFG_1, .port = 0x7, .id_mask = 0xFC0,
		.id_val = 0x800, .note = "MFG", .name = "MFG_m_57" },

	/* PERI */
	{ .master = MST_ID_PERI_0,   .port = 0x7, .id_mask = 0xFF3,
		.id_val = 0x0, .note = "MSDC0", .name = "MSDC0_m_57" },
	{ .master = MST_ID_PERI_1,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x2, .note = "PWM", .name = "PWM_m_57" },
	{ .master = MST_ID_PERI_2,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x6, .note = "MSDC1", .name = "MSDC1_m_57" },
	{ .master = MST_ID_PERI_3,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0xA, .note = "MSDC2", .name = "MSDC2_m_57" },
	{ .master = MST_ID_PERI_4,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0xE, .note = "SPI0", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_5,   .port = 0x7, .id_mask = 0xFDF,
		.id_val = 0x1, .note = "Debug Top", .name = "DebugTop_m_57" },
	{ .master = MST_ID_PERI_6,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x5, .note = "AUDIO", .name = "AUDIO_m_57" },
	{ .master = MST_ID_PERI_7,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x105, .note = "MSDC3", .name = "MSDC0_m_57" },
	{ .master = MST_ID_PERI_8,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x205, .note = "SPI1", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_9,   .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x305, .note = "USB2.0", .name = "USB2.0_m_57" },
	{ .master = MST_ID_PERI_10,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x125, .note = "SPM1_SPM2", .name = "SPM_m_57" },
	{ .master = MST_ID_PERI_11,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x325, .note = "THERM", .name = "THERM_m_57" },
	{ .master = MST_ID_PERI_12,  .port = 0x7, .id_mask = 0xEFF,
		.id_val = 0x45, .note = "U3", .name = "U3_m_57" },
	{ .master = MST_ID_PERI_13,  .port = 0x7, .id_mask = 0xEFF,
		.id_val = 0x65, .note = "DMA", .name = "DMA_m_57" },
	{ .master = MST_ID_PERI_14,  .port = 0x7, .id_mask = 0x9FF,
		.id_val = 0x85, .note = "MSDC0", .name = "MSDC0_m_57" },
	{ .master = MST_ID_PERI_15,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x185, .note = "PWM", .name = "PWM_m_57" },
	{ .master = MST_ID_PERI_16,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x385, .note = "MSDC1", .name = "MSDC1_m_57" },
	{ .master = MST_ID_PERI_17,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x585, .note = "MSDC2", .name = "MSDC2_m_57" },
	{ .master = MST_ID_PERI_18,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x785, .note = "SPI0", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_19,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0xA5, .note = "SPI2", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_20,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x1A5, .note = "SPI3", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_21,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x2A5, .note = "SPI4", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_22,  .port = 0x7, .id_mask = 0xFFF,
		.id_val = 0x3A5, .note = "SPI5", .name = "SPI_m_57" },
	{ .master = MST_ID_PERI_23,  .port = 0x7, .id_mask = 0xFDF,
		.id_val = 0x9, .note = "CONNSYS", .name = "CONNSYS_m_57" },
	{ .master = MST_ID_PERI_24,  .port = 0x7, .id_mask = 0xFDF,
		.id_val = 0xD, .note = "GCPU_M", .name = "GCPU_M_m_57" },
	{ .master = MST_ID_PERI_25,  .port = 0x7, .id_mask = 0xFDF,
		.id_val = 0x11, .note = "CLDMA", .name = "CLDMA_AP_m_57" },
	{ .master = MST_ID_PERI_26,  .port = 0x7, .id_mask = 0xFF3,
		.id_val = 0x3, .note = "GCE_M", .name = "GCE_M_m_57" },

};

static const char *UNKNOWN_MASTER = "unknown";
static DEFINE_SPINLOCK(emi_mpu_lock);

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
char *smi_larb3_port[13] = {
	"LARB3_VENC_m_57", "LARB3_VENC_m_57", "LARB3_VENC_m_57",
	"LARB3_VENC_m_57", "LARB3_VENC_m_57", "LARB3_JPEG_m_57",
	"LARB3_JPEG_m_57", "LARB3_JPEG_m_57", "LARB3_JPEG_m_57",
	"LARB3_VENC_m_57", "LARB3_VENC_m_57", "LARB3_VENC_m_57",
	"LARB3_VENC_m_57"};
char *smi_larb4_port[11] = {
	"LARB4_display_m_57", "LARB4_display_m_57", "LARB4_display_m_57",
	"LARB4_display_m_57", "LARB4_display_m_57", "LARB4_display_m_57",
	"LARB4_display_m_57", "LARB4_display_m_57", "LARB4_MDP_m_57",
	"LARB4_MDP_m_57", "LARB4_display_m_57"};
char *smi_larb5_port[14] = {
	"LARB5_IMG_m_57", "LARB5_IMG_m_57", "LARB5_IMG_m_57",
	"LARB5_IMG_m_57", "LARB5_IMG_m_57", "LARB5_IMG1_m_57",
	"LARB5_IMG1_m_57", "LARB5_IMG1_m_57", "LARB5_IMG_DPE_m_57",
	"LARB5_IMG_DPE_m_57", "LARB5_IMG_RSP_m_57", "LARB5_IMG_RSP_m_57",
	"LARB5_IMG1_m_57", "LARB5_IMG1_m_57"};

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID)
{
	u32 mm_larb;
	u32 smi_port;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val)
		&& (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case MASTER_APMCU_CH1:
		case MASTER_APMCU_CH2:
		case MASTER_MDMCU:
		case MASTER_MDDMA:
		case MASTER_MFG:
		case MASTER_PERI:
			pr_err("Violation master name is %s.\n",
			mst_tbl[tbl_idx].name);
			break;
		case MASTER_MM_CH1:
		case MASTER_MM_CH2:
			mm_larb = axi_id >> 7;
			smi_port = (axi_id & 0x7F) >> 2;
			/* smi_larb0 */
			if (mm_larb == 0x0) {
				if (smi_port >= ARRAY_SIZE(smi_larb0_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb0_port[smi_port]);
			/* smi_larb1 */
			} else if (mm_larb == 0x1) {
				if (smi_port >= ARRAY_SIZE(smi_larb1_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb1_port[smi_port]);
			/* smi_larb2 */
			} else if (mm_larb == 0x2) {
				if (smi_port >= ARRAY_SIZE(smi_larb2_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb2_port[smi_port]);
			/* smi_larb3 */
			} else if (mm_larb == 0x3) {
				if (smi_port >= ARRAY_SIZE(smi_larb3_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb3_port[smi_port]);
			/* smi_larb4 */
			} else if (mm_larb == 0x4) {
				if (smi_port >= ARRAY_SIZE(smi_larb4_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb4_port[smi_port]);
			/* smi_larb5 */
			} else if (mm_larb == 0x5) {
				if (smi_port >= ARRAY_SIZE(smi_larb5_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name, smi_larb5_port[smi_port]);
			} else {	/*MM IOMMU Internal Used */
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

	pr_err("[EMI MPU] emi_mpu_get_violation_port\n");

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

	axi_ID = (id >> 3) & 0x00001FFF;
	port_ID = id & 0x00000007;
	emi_mpu_set_violation_port(port_ID);
	pr_err("[EMI MPU] axi_id = %x, port_id = %x\n", axi_ID, port_ID);

	for (i = 0; i < ARRAY_SIZE(mst_tbl); i++) {
		if (__match_id(axi_ID, i, port_ID))	{
			if ((port_ID == MASTER_MM_CH1) || (port_ID == MASTER_MM_CH2)) {
				mm_larbID = axi_ID >> 7;
				smi_portID = (axi_ID & 0x7F) >> 2;
				if (mm_larbID == 0x0)
					return smi_larb0_port[smi_portID];
				else if (mm_larbID == 0x1)
					return smi_larb1_port[smi_portID];
				else if (mm_larbID == 0x2)
					return smi_larb2_port[smi_portID];
				else if (mm_larbID == 0x3)
					return smi_larb3_port[smi_portID];
				else if (mm_larbID == 0x4)
					return smi_larb4_port[smi_portID];
				else if (mm_larbID == 0x5)
					return smi_larb5_port[smi_portID];
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
		pr_err("Fail to clear EMI MPU violation\n");
		pr_err("EMI_MPUS = %x, EMI_MPUT = %x", dbg_s, dbg_t);
	}
}

static int mpu_check_violation(void)
{
	u32 dbg_s, dbg_t, dbg_t_2nd;
	u32 master_ID, domain_ID, wr_vio, wr_oo_vio;
	s32 region;
	char *master_name;

	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));
	dbg_t_2nd = readl(IOMEM(EMI_MPUT_2ND));
	vio_addr = (dbg_t + (((unsigned long long)(dbg_t_2nd & 0xF)) << 32) + emi_physical_offset);

	pr_alert("Clear status.\n");

	master_ID = (dbg_s & 0x0000FFFF);
	domain_ID = (dbg_s >> 21) & 0x0000000F;
	wr_vio = (dbg_s >> 29) & 0x00000003;
	wr_oo_vio = (dbg_s >> 27) & 0x00000003;
	region = (dbg_s >> 16) & 0x1F;

	/*TBD: print the abort region*/

	pr_err("EMI MPU violation.\n");
	pr_err("[EMI MPU] Debug info start ----------------------------------------\n");

	pr_err("EMI_MPUS = %x, EMI_MPUT = %x, EMI_MPUT_2ND = %x.\n", dbg_s, dbg_t, dbg_t_2nd);
	pr_err("Current process is \"%s \" (pid: %i).\n", current->comm, current->pid);
	pr_err("Violation address is 0x%llx.\n", vio_addr);
	pr_err("Violation master ID is 0x%x.\n", master_ID);
	/*print out the murderer name*/
	master_name = __id2name(master_ID);
	pr_err("Violation domain ID is 0x%x.\n", domain_ID);
	if (wr_vio == 1)
		pr_err("Write violation.\n");
	else if (wr_vio == 2)
		pr_err("Read violation.\n");
	else
		pr_err("Strange write / read violation value = %d.\n", wr_vio);
	pr_err("Corrupted region is %d\n\r", region);
	if (wr_oo_vio == 1)
		pr_err("Write out of range violation.\n");
	else if (wr_oo_vio == 2)
		pr_err("Read out of range violation.\n");

	pr_err("[EMI MPU] Debug info end------------------------------------------\n");

#ifdef CONFIG_MTK_AEE_FEATURE
	if (wr_vio != 0) {
		if (((master_ID & 0x7) == MASTER_MDMCU) || ((master_ID & 0x7) == MASTER_MDDMA)) {
			char str[60] = "0";
			char *pstr = str;

			sprintf(pstr, "EMI_MPUS = 0x%x, ADDR = 0x%llx", dbg_s, vio_addr);

			exec_ccci_kern_func_by_md_id(0, ID_MD_MPU_ASSERT, str, strlen(str));
			pr_err("[EMI MPU] MPU violation trigger MD str=%s strlen(str)=%d\n",
				str, (int)strlen(str));
		} else {
			exec_ccci_kern_func_by_md_id(0, ID_MD_MPU_ASSERT, NULL, 0);
			pr_err("[EMI MPU] MPU violation ack to MD\n");
		}

		aee_kernel_exception("EMI MPU",
"%sEMI_MPUS = 0x%x,EMI_MPUT = 0x%x,EMI_MPUT_2ND = 0x%x,vio_addr = 0x%llx\n CHKER = 0x%x,CHKER_TYPE = 0x%x,CHKER_ADR = 0x%x\n%s%s\n",
		     "EMI MPU violation.\n",
		     dbg_s, dbg_t, dbg_t_2nd, vio_addr,
		     readl(IOMEM(EMI_CHKER)),
		     readl(IOMEM(EMI_CHKER_TYPE)),
		     readl(IOMEM(EMI_CHKER_ADR)),
		     "CRDISPATCH_KEY:EMI MPU Violation Issue/",
		     master_name);
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
unsigned long long end, int region, unsigned long long access_permission)
{
	int ret = 0;
	unsigned long flags;

	access_permission = access_permission & 0x00FFFFFF04FFFFFF;
	access_permission = access_permission | (((unsigned long long)region & 0x1F)<<27);
	spin_lock_irqsave(&emi_mpu_lock, flags);
	emi_mpu_smc_set(start, end, access_permission);
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
		start = ((unsigned long long)mt_emi_reg_read(EMI_MPU_SA(region)) << 16) + emi_physical_offset;
		end = ((unsigned long long)mt_emi_reg_read(EMI_MPU_EA(region)) << 16) + emi_physical_offset;
		ptr += sprintf(ptr, "R%d-> 0x%llx to 0x%llx\n", region, start, end+0xFFFF);

		/* for(i=0; i<(EMI_MPU_DOMAIN_NUMBER/8); i++) { */
		/* only print first 8 domain */
		for (i = 0; i < 2; i++) {
			reg_value = mt_emi_reg_read(EMI_MPU_APC(region, i*8));
			for (j = 0; j < 8; j++) {
				ptr += sprintf(ptr, "%s, ", permission[((reg_value >> (j*3)) & 0x7)]);
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
		pr_err("mpu_test_buffer+10000 = 0x%x\n", temp);
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
	unsigned long long access_permission;
	char *command;
	char *ptr;
	char *token[5];
	ssize_t ret;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("emi_mpu_store command overflow.");
		return count;
	}
	pr_err("emi_mpu_store: %s\n", buf);

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
		ret = kstrtoull(token[4], 16, (unsigned long long *)&access_permission);
		if (ret != 0)
			pr_err("kstrtoull fails to parse access_permission");
		emi_mpu_set_region_protection(start_addr, end_addr, region, access_permission);
		pr_err("Set EMI_MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%llx.\n",
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
		SET_ACCESS_PERMISSON(UNLOCK,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));

		pr_err("set EMI MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%llx\n",
		0, 0, region,
		SET_ACCESS_PERMISSON(UNLOCK,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION,
			NO_PROTECTION, NO_PROTECTION, NO_PROTECTION, NO_PROTECTION));
	} else {
		pr_err("Unknown emi_mpu command.\n");
	}

	kfree(command);

	return count;
}

DRIVER_ATTR(mpu_config, 0644, emi_mpu_show, emi_mpu_store);

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
		pr_err("AXI violation.\n");
		pr_err("[EMI MPU AXI] Debug info start ----------------------------------------\n");

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
		master_name = __id2name(master_ID);
		pr_err("violation master_name is %s.\n", master_name);

		pr_err("[EMI MPU AXI] Debug info end ----------------------------------------\n");

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

DRIVER_ATTR(emi_axi_vio, 0644, emi_axi_vio_show, emi_axi_vio_store);

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
	pr_err("[EMI_WP] Violation Address is : 0x%llx\n",
	(readl(IOMEM(EMI_CHKER_ADR)) + (((unsigned long long)(readl(IOMEM(EMI_CHKER_ADR_2ND)) & 0xF)) << 32)
	+ emi_physical_offset));

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
	pr_err("emi_wp_store: %s\n", buf);

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
	unsigned int mpu_irq = 0;

#if EMI_MPU_TEST
	unsigned int *ptr0_phyical;

	ptr0_phyical = (unsigned int *)__pa(mpu_test_buffer);
	pr_err("[EMI_MPU]   ptr0_phyical : %p\n", ptr0_phyical);
	*((volatile unsigned int *)(mpu_test_buffer+0x10000)) = 0xdeaddead;
#endif

	pr_info("Initialize EMI MPU.\n");

	isetocunt = 0;

	/* DTS version */
	CEN_EMI_BASE = mt_cen_emi_base_get();
	CHA_EMI_BASE = mt_chn_emi_base_get(0);
	CHB_EMI_BASE = mt_chn_emi_base_get(1);
	EMI_MPU_BASE = mt_emi_mpu_base_get();

	if (CEN_EMI_BASE == NULL) {
		pr_err("[EMI MPU] can't get CEN_EMI_BASE\n");
		return -1;
	}

	if (CHA_EMI_BASE == NULL) {
		pr_err("[EMI MPU] can't get CHA_EMI_BASE\n");
		return -1;
	}

	if (CHB_EMI_BASE == NULL) {
		pr_err("[EMI MPU] can't get CHB_EMI_BASE\n");
		return -1;
	}

	if (EMI_MPU_BASE == NULL) {
		pr_err("[EMI MPU] can't get EMI_MPU_BASE\n");
		return -1;
	}

	mt_emi_reg_base_set(CEN_EMI_BASE);

	pr_err("[EMI MPU] EMI_MPUS = 0x%x\n", readl(IOMEM(EMI_MPUS)));
	pr_err("[EMI MPU] EMI_MPUT = 0x%x\n", readl(IOMEM(EMI_MPUT)));
	pr_err("[EMI MPU] EMI_MPUT_2ND = 0x%x\n", readl(IOMEM(EMI_MPUT_2ND)));

	pr_err("[EMI MPU] EMI_WP_ADR = 0x%x\n", readl(IOMEM(EMI_WP_ADR)));
	pr_err("[EMI MPU] EMI_WP_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_WP_ADR_2ND)));
	pr_err("[EMI MPU] EMI_WP_CTRL = 0x%x\n", readl(IOMEM(EMI_WP_CTRL)));
	pr_err("[EMI MPU] EMI_CHKER = 0x%x\n", readl(IOMEM(EMI_CHKER)));
	pr_err("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n", readl(IOMEM(EMI_CHKER_TYPE)));
	pr_err("[EMI MPU] EMI_CHKER_ADR = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR)));
	pr_err("[EMI MPU] EMI_CHKER_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_CHKER_ADR_2ND)));

	if (readl(IOMEM(EMI_MPUS))) {
		pr_err("[EMI MPU] get MPU violation in driver init\n");
		mpu_check_violation();
	} else {
		__clear_emi_mpu_vio(0);
	}

	emi_physical_offset = 0x40000000;

	/*
	 * NoteXXX: Interrupts of violation (including SPC in SMI, or EMI MPU)
	 *		  are triggered by the device APC.
	 *		  Need to share the interrupt with the SPC driver.
	 */
	mpu_irq = mt_emi_mpu_irq_get();
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
	mt_reg_sync_writel((1 << AXI_VIO_CLR) | readl(IOMEM(EMI_CHKER)), EMI_CHKER);
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

#ifdef ENABLE_EMI_WATCH_POINT
	ret = driver_create_file(&emi_mpu_ctrl.driver, &driver_attr_emi_wp_vio);
	if (ret)
		pr_err("Fail to create WP violation monitor sysfs file.\n");

#endif
#endif

	return 0;
}

static void __exit emi_mpu_mod_exit(void)
{
}

module_init(emi_mpu_mod_init);
module_exit(emi_mpu_mod_exit);

