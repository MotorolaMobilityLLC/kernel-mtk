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
#include <mt-plat/mtk_ram_console.h>
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
bool is_dump_emi_latency;

#define EMI_MPU_TEST	0
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
void __iomem *CHN0_EMI_BASE_ADDR;
void __iomem *CHN1_EMI_BASE_ADDR;
void __iomem *CHN2_EMI_BASE_ADDR;
void __iomem *CHN3_EMI_BASE_ADDR;
void __iomem *INFRA_BASE_ADDR;
static void __iomem *INFRA_CFG;
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
	/* APMCU CH1 */
	{.master = MID_APMCU_CH1_0, .port = 0x0, .id_mask = 0x1FE7,
		.id_val = 0x0000,
		.note = "Mercury: Core nn system domain load/store exclusive",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH1_1, .port = 0x0, .id_mask = 0x1FE7,
		.id_val = 0x0060,
		.note = "Mercury: Core nn non-re-orderable device write",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH1_2, .port = 0x0, .id_mask = 0x1F87,
		.id_val = 0x0080,
		.note = "Mercury: Write to normal memory, or re-orderable device memory",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH1_3, .port = 0x0, .id_mask = 0x1807,
		.id_val = 0x1000,
		.note = "Mercury: MCSIB write to ensure clean",
		.name = "Mercury_M99"},

	{.master = MID_APMCU_CH1_4, .port = 0x0, .id_mask = 0x1F9F,
		.id_val = 0x0080,
		.note = "Mercury: ACP read",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH1_5, .port = 0x0, .id_mask = 0x1F07,
		.id_val = 0x0100,
		.note = "Mercury: Core nn read",
		.name = "Mercury_M99"},

	{.master = MID_APMCU_CH1_6, .port = 0x0, .id_mask = 0x1FE7,
		.id_val = 0x0001,
		.note = "CA53: Core nn system domain load/store exclusive",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH1_7, .port = 0x0, .id_mask = 0x1FE7,
		.id_val = 0x0021,
		.note = "CA53: Core nn barrier",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH1_8, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x0049,
		.note = "CA53: SCU generated barrier or DVM complete",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH1_9, .port = 0x0, .id_mask = 0x1FE7,
		.id_val = 0x0061,
		.note = "CA53: Core nn non-re-orderable device write",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH1_10, .port = 0x0, .id_mask = 0x1F87,
		.id_val = 0x0081,
		.note = "CA53: Write to normal memory, or re-orderable device memory",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH1_11, .port = 0x0, .id_mask = 0x1807,
		.id_val = 0x1001,
		.note = "CA53: MCSIB write to ensure clean",
		.name = "CA53_M99"},

	{.master = MID_APMCU_CH1_12, .port = 0x0, .id_mask = 0x1F9F,
		.id_val = 0x0081,
		.note = "CA53: ACP read",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH1_13, .port = 0x0, .id_mask = 0x1F07,
		.id_val = 0x0101,
		.note = "CA53: Core nn read",
		.name = "CA53_M99"},

	{.master = MID_APMCU_CH1_14, .port = 0x0, .id_mask = 0x1F87,
		.id_val = 0x0782,
		.note = "CA57A: L2 Evictions",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_15, .port = 0x0, .id_mask = 0x1C97,
		.id_val = 0x0012,
		.note = "CA57A: Normal non-cacheable",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_16, .port = 0x0, .id_mask = 0x1CFF,
		.id_val = 0x0002,
		.note = "CA57A: SO or TLB NC",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_17, .port = 0x0, .id_mask = 0x1CFF,
		.id_val = 0x0009,
		.note = "CA57A: DV",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_18, .port = 0x0, .id_mask = 0x1CFF,
		.id_val = 0x0022,
		.note = "CA57A: Exclusive or Data NC/SO/Dev",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_19, .port = 0x0, .id_mask = 0x1807,
		.id_val = 0x1002,
		.note = "CA57A: MCSIB write to ensure clean",
		.name = "CA57A_M99"},

	{.master = MID_APMCU_CH1_20, .port = 0x0, .id_mask = 0x1CFF,
		.id_val = 0x0042,
		.note = "CA57A: Instr NC",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_21, .port = 0x0, .id_mask = 0x1E87,
		.id_val = 0x0082,
		.note = "CA57A: L2 Linefill",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_22, .port = 0x0, .id_mask = 0x1F87,
		.id_val = 0x0282,
		.note = "CA57A: L2 Linefill",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_23, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x0742,
		.note = "CA57A: DVM",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_24, .port = 0x0, .id_mask = 0x1F87,
		.id_val = 0x0582,
		.note = "CA57A: STB Make Unique",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_25, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x0522,
		.note = "CA57A: CMO Clean Unique",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_26, .port = 0x0, .id_mask = 0x1FE7,
		.id_val = 0x0542,
		.note = "CA57A: Store EX (CleanUnique)",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_27, .port = 0x0, .id_mask = 0x1F07,
		.id_val = 0x0602,
		.note = "CA57A: Invalidate (CleanUnique)",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH1_28, .port = 0x0, .id_mask = 0x1FC7,
		.id_val = 0x0702,
		.note = "CA57A: Invalidate (CleanUnique)",
		.name = "CA57A_M99"},

	/* APMCU CH2 */
	{.master = MID_APMCU_CH2_0, .port = 0x1, .id_mask = 0x1FE7,
		.id_val = 0x0000,
		.note = "Mercury: Core nn system domain load/store exclusive",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH2_1, .port = 0x1, .id_mask = 0x1FE7,
		.id_val = 0x0060,
		.note = "Mercury: Core nn non-re-orderable device write",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH2_2, .port = 0x1, .id_mask = 0x1F87,
		.id_val = 0x0080,
		.note = "Mercury: Write to normal memory, or re-orderable device memory",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH2_3, .port = 0x1, .id_mask = 0x1807,
		.id_val = 0x1000,
		.note = "Mercury: MCSIB write to ensure clean",
		.name = "Mercury_M99"},

	{.master = MID_APMCU_CH2_4, .port = 0x1, .id_mask = 0x1F9F,
		.id_val = 0x0080,
		.note = "Mercury: ACP read",
		.name = "Mercury_M99"},
	{.master = MID_APMCU_CH2_5, .port = 0x1, .id_mask = 0x1F07,
		.id_val = 0x0100,
		.note = "Mercury: Core nn read",
		.name = "Mercury_M99"},

	{.master = MID_APMCU_CH2_6, .port = 0x1, .id_mask = 0x1FE7,
		.id_val = 0x0001,
		.note = "CA53: Core nn system domain load/store exclusive",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH2_7, .port = 0x1, .id_mask = 0x1FE7,
		.id_val = 0x0021,
		.note = "CA53: Core nn barrier",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH2_8, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x0049,
		.note = "CA53: SCU generated barrier or DVM complete",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH2_9, .port = 0x1, .id_mask = 0x1FE7,
		.id_val = 0x0061,
		.note = "CA53: Core nn non-re-orderable device write",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH2_10, .port = 0x1, .id_mask = 0x1F87,
		.id_val = 0x0081,
		.note = "CA53: Write to normal memory, or re-orderable device memory",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH2_11, .port = 0x1, .id_mask = 0x1807,
		.id_val = 0x1001,
		.note = "CA53: MCSIB write to ensure clean",
		.name = "CA53_M99"},

	{.master = MID_APMCU_CH2_12, .port = 0x1, .id_mask = 0x1F9F,
		.id_val = 0x0081,
		.note = "CA53: ACP read",
		.name = "CA53_M99"},
	{.master = MID_APMCU_CH2_13, .port = 0x1, .id_mask = 0x1F07,
		.id_val = 0x0101,
		.note = "CA53: Core nn read",
		.name = "CA53_M99"},

	{.master = MID_APMCU_CH2_14, .port = 0x1, .id_mask = 0x1F87,
		.id_val = 0x0782,
		.note = "CA57A: L2 Evictions",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_15, .port = 0x1, .id_mask = 0x1C97,
		.id_val = 0x0012,
		.note = "CA57A: Normal non-cacheable",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_16, .port = 0x1, .id_mask = 0x1CFF,
		.id_val = 0x0002,
		.note = "CA57A: SO or TLB NC",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_17, .port = 0x1, .id_mask = 0x1CFF,
		.id_val = 0x0009,
		.note = "CA57A: DV",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_18, .port = 0x1, .id_mask = 0x1CFF,
		.id_val = 0x0022,
		.note = "CA57A: Exclusive or Data NC/SO/Dev",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_19, .port = 0x1, .id_mask = 0x1807,
		.id_val = 0x1002,
		.note = "CA57A: MCSIB write to ensure clean",
		.name = "CA57A_M99"},

	{.master = MID_APMCU_CH2_20, .port = 0x1, .id_mask = 0x1CFF,
		.id_val = 0x0042,
		.note = "CA57A: Instr NC",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_21, .port = 0x1, .id_mask = 0x1E87,
		.id_val = 0x0082,
		.note = "CA57A: L2 Linefill",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_22, .port = 0x1, .id_mask = 0x1F87,
		.id_val = 0x0282,
		.note = "CA57A: L2 Linefill",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_23, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x0742,
		.note = "CA57A: DVM",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_24, .port = 0x1, .id_mask = 0x1F87,
		.id_val = 0x0582,
		.note = "CA57A: STB Make Unique",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_25, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x0522,
		.note = "CA57A: CMO Clean Unique",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_26, .port = 0x1, .id_mask = 0x1FE7,
		.id_val = 0x0542,
		.note = "CA57A: Store EX (CleanUnique)",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_27, .port = 0x1, .id_mask = 0x1F07,
		.id_val = 0x0602,
		.note = "CA57A: Invalidate (CleanUnique)",
		.name = "CA57A_M99"},
	{.master = MID_APMCU_CH2_28, .port = 0x1, .id_mask = 0x1FC7,
		.id_val = 0x0702,
		.note = "CA57A: Invalidate (CleanUnique)",
		.name = "CA57A_M99"},

	/* MM CH1 */
	{.master = MID_MM_CH1_0, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0000,
		.note = "MM: LARB0",
		.name = "MM_LARB0_M99"},
	{.master = MID_MM_CH1_1, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0080,
		.note = "MM: LARB1",
		.name = "MM_LARB1_M99"},
	{.master = MID_MM_CH1_2, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0100,
		.note = "MM_VDEC: LARB4",
		.name = "MM_LARB4_M99"},
	{.master = MID_MM_CH1_3, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0180,
		.note = "MM_IMG: LARB5",
		.name = "MM_LARB5_M99"},
	{.master = MID_MM_CH1_4, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0200,
		.note = "MM_CAM: LARB6",
		.name = "MM_LARB6_M99"},
	{.master = MID_MM_CH1_5, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0300,
		.note = "MM_VENC: LARB7",
		.name = "MM_LARB7_M99"},
	{.master = MID_MM_CH1_6, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0280,
		.note = "MM_MJC: LARB8",
		.name = "MM_LARB8_M99"},
	{.master = MID_MM_CH1_7, .port = 0x2, .id_mask = 0x1F80,
		.id_val = 0x0380,
		.note = "MM_IPU: IPU",
		.name = "MM_IPU_M99"},
	{.master = MID_MM_CH1_8, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x07FC,
		.note = "MM_M4U: MM IOMMU Internal Used",
		.name = "MM_IOMMU_M99"},
	{.master = MID_MM_CH1_9, .port = 0x2, .id_mask = 0x1FFF,
		.id_val = 0x07FD,
		.note = "MM_M4U: MM IOMMU Internal Used",
		.name = "MM_IOMMU_M99"},

	/* MDMCU */
	{.master = MID_MDMCU_0, .port = 0x3, .id_mask = 0x1FC3,
		.id_val = 0x0000,
		.note = "MDMCU: MDMCU cache read",
		.name = "MDMCU_CACHE_M99"},
	{.master = MID_MDMCU_1, .port = 0x3, .id_mask = 0x1FFF,
		.id_val = 0x001C,
		.note = "MDMCU: MDMCU cache write",
		.name = "MDMCU_CACHE_M99"},
	{.master = MID_MDMCU_2, .port = 0x3, .id_mask = 0x1FF3,
		.id_val = 0x0040,
		.note = "MDMCU: MDMCU noncache",
		.name = "MDMCU_NONCACHE_M99"},
	{.master = MID_MDMCU_3, .port = 0x3, .id_mask = 0x1FFF,
		.id_val = 0x0050,
		.note = "MDMCU: MDMCU noncache",
		.name = "MDMCU_NONCACHE_M99"},
	{.master = MID_MDMCU_4, .port = 0x3, .id_mask = 0x1FF3,
		.id_val = 0x0060,
		.note = "MDMCU: MDMCU noncache",
		.name = "MDMCU_NONCACHE_M99"},

	/* C2K */
	{.master = MID_C2K_0, .port = 0x3, .id_mask = 0x1FFF,
		.id_val = 0x0011,
		.note = "C2K: ARM I",
		.name = "C2K_ARM_I_M99"},
	{.master = MID_C2K_1, .port = 0x3, .id_mask = 0x1FFF,
		.id_val = 0x0005,
		.note = "C2K: ARM D",
		.name = "C2K_ARM_D_M99"},
	{.master = MID_C2K_2, .port = 0x3, .id_mask = 0x1FFF,
		.id_val = 0x0025,
		.note = "C2K: DMA",
		.name = "C2K_DMA_M99"},
	{.master = MID_C2K_3, .port = 0x3, .id_mask = 0x1FFF,
		.id_val = 0x0019,
		.note = "C2K: prefetch buffer",
		.name = "C2K_PREFETCH_M99"},

	/* INFRA */
	{.master = MID_INFRA_0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0106,
		.note = "Infra/Peri: audio",
		.name = "PERI_AUDIO_M99"},
	{.master = MID_INFRA_1, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0101,
		.note = "Infra/Peri: cldma_ap",
		.name = "PERI_CLDMA_AP_M99"},
	{.master = MID_INFRA_2, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0105,
		.note = "Infra/Peri: cldma_md",
		.name = "PERI_CLDMA_MD_M99"},
	{.master = MID_INFRA_3, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x003E,
		.note = "Infra/Peri: pwm",
		.name = "PERI_PWM_M99"},
	{.master = MID_INFRA_4, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x010E,
		.note = "Infra/Peri: SSPM",
		.name = "PERI_SSPM_M99"},
	{.master = MID_INFRA_5, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0112,
		.note = "Infra/Peri: scp",
		.name = "PERI_SCP_M99"},
	{.master = MID_INFRA_6, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0022,
		.note = "Infra/Peri: spi0",
		.name = "PERI_SPI0_M99"},
	{.master = MID_INFRA_7, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0026,
		.note = "Infra/Peri: spi1",
		.name = "PERI_SPI1_M99"},
	{.master = MID_INFRA_8, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x002A,
		.note = "Infra/Peri: spi2",
		.name = "PERI_SPI2_M99"},
	{.master = MID_INFRA_9, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x002E,
		.note = "Infra/Peri: spi3",
		.name = "PERI_SPI3_M99"},
	{.master = MID_INFRA_10, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0032,
		.note = "Infra/Peri: spi4",
		.name = "PERI_SPI4_M99"},
	{.master = MID_INFRA_11, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0092,
		.note = "Infra/Peri: spi5 and spi6",
		.name = "PERI_SPI5_SPI6_M99"},
	{.master = MID_INFRA_12, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0096,
		.note = "Infra/Peri: spi7",
		.name = "PERI_SPI7_M99"},
	{.master = MID_INFRA_13, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x009A,
		.note = "Infra/Peri: spi8",
		.name = "PERI_SPI8_M99"},
	{.master = MID_INFRA_14, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x009E,
		.note = "Infra/Peri: spi9",
		.name = "PERI_SPI9_M99"},
	{.master = MID_INFRA_15, .port = 0x4, .id_mask = 0x1FF0,
		.id_val = 0x0060,
		.note = "Infra/Peri: ufo_encode_decode",
		.name = "PERI_UFO_ENDEC_M99"},
	{.master = MID_INFRA_16, .port = 0x4, .id_mask = 0x1FF9,
		.id_val = 0x0040,
		.note = "Infra/Peri: UFSHCI",
		.name = "PERI_UFSHCI_M99"},
	{.master = MID_INFRA_17, .port = 0x4, .id_mask = 0x1FF3,
		.id_val = 0x00A0,
		.note = "Infra/Peri: ssusb_dual",
		.name = "PERI_SSUSB_DUAL_M99"},
	{.master = MID_INFRA_18, .port = 0x4, .id_mask = 0x1FF3,
		.id_val = 0x00A1,
		.note = "Infra/Peri: ssusb_xhci",
		.name = "PERI_SSUSB_XHCI_M99"},
	{.master = MID_INFRA_19, .port = 0x4, .id_mask = 0x1FFB,
		.id_val = 0x0020,
		.note = "Infra/Peri: MSDC0",
		.name = "PERI_MSDC0_M99"},
	{.master = MID_INFRA_20, .port = 0x4, .id_mask = 0x1FFB,
		.id_val = 0x0023,
		.note = "Infra/Peri: MSDC1",
		.name = "PERI_MSDC1_M99"},
	{.master = MID_INFRA_21, .port = 0x4, .id_mask = 0x1FFB,
		.id_val = 0x0081,
		.note = "Infra/Peri: MSDC3",
		.name = "PERI_MSDC3_M99"},
	{.master = MID_INFRA_22, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0089,
		.note = "Infra/Peri: MSDC3",
		.name = "PERI_MSDC3_M99"},
	{.master = MID_INFRA_23, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0021,
		.note = "Infra/Peri: NOR-Flash",
		.name = "PERI_NORFLASH_M99"},
	{.master = MID_INFRA_24, .port = 0x4, .id_mask = 0x1FE1,
		.id_val = 0x00C0,
		.note = "Infra/Peri: DXCC_64P",
		.name = "PERI_DXCC64P_M99"},
	{.master = MID_INFRA_25, .port = 0x4, .id_mask = 0x1FE1,
		.id_val = 0x00C1,
		.note = "Infra/Peri: DXCC_64S",
		.name = "PERI_DXCC64S_M99"},
	{.master = MID_INFRA_26, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0000,
		.note = "Infra/Peri: DBGTOP",
		.name = "PERI_DBGTOP_M99"},
	{.master = MID_INFRA_27, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0006,
		.note = "Infra/Peri: DBGTOP",
		.name = "PERI_DBGTOP_M99"},
	{.master = MID_INFRA_28, .port = 0x4, .id_mask = 0x1FF1,
		.id_val = 0x0001,
		.note = "Infra/Peri: GDMA (CQDMA)",
		.name = "PERI_CQDMA_M99"},
	{.master = MID_INFRA_29, .port = 0x4, .id_mask = 0x1FFB,
		.id_val = 0x0100,
		.note = "Infra/Peri: GCE",
		.name = "PERI_GCE_M99"},
	{.master = MID_INFRA_30, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0108,
		.note = "Infra/Peri: GCE",
		.name = "PERI_GCE_M99"},
	{.master = MID_INFRA_31, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0116,
		.note = "Infra/Peri: thermal",
		.name = "PERI_THERMAL_M99"},
	{.master = MID_INFRA_32, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x010A,
		.note = "Infra/Peri: spm",
		.name = "PERI_SPM_M99"},
	{.master = MID_INFRA_33, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0082,
		.note = "Infra/Peri: usb2.0",
		.name = "PERI_USB2_M99"},
	{.master = MID_INFRA_34, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0083,
		.note = "Infra/Peri: pcie_mac",
		.name = "PERI_PCIE_M99"},
	{.master = MID_INFRA_35, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0080,
		.note = "Infra/Peri: apdma",
		.name = "PERI_APDMA_M99"},

	/* MDDMA */
	{.master = MID_MDDMA_0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x1006,
		.note = "MDDMA: MDDBGSYS",
		.name = "MDDMA_MDDBGSYS_M99"},
	{.master = MID_MDDMA_1, .port = 0x4, .id_mask = 0x1FFB,
		.id_val = 0x1001,
		.note = "MDDMA: SOE",
		.name = "MDDMA_SOE_M99"},
	{.master = MID_MDDMA_2, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x1000,
		.note = "MDDMA: GDMALOG",
		.name = "MDDMA_GDMALOG_M99"},
	{.master = MID_MDDMA_3, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x1200,
		.note = "MDDMA: GDMA",
		.name = "MDDMA_GDMA_M99"},
	{.master = MID_MDDMA_4, .port = 0x4, .id_mask = 0x1FFE,
		.id_val = 0x1400,
		.note = "MDDMA: HSPAL2",
		.name = "MDDMA_HSPAL2_M99"},
	{.master = MID_MDDMA_5, .port = 0x4, .id_mask = 0x1E00,
		.id_val = 0x1600,
		.note = "MDDMA: L1SYS2MDA",
		.name = "MDDMA_L1SYS2MDA_M99"},
	{.master = MID_MDDMA_6, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x1801,
		.note = "MDDMA: ASM0",
		.name = "MDDMA_ASM0_M99"},
	{.master = MID_MDDMA_7, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x1805,
		.note = "MDDMA: ASM1",
		.name = "MDDMA_ASM1_M99"},
	{.master = MID_MDDMA_8, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x1809,
		.note = "MDDMA: ASM2",
		.name = "MDDMA_ASM2_M99"},
	{.master = MID_MDDMA_9, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x180D,
		.note = "MDDMA: ASM3",
		.name = "MDDMA_ASM3_M99"},
	{.master = MID_MDDMA_10, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x1803,
		.note = "MDDMA: ASM4",
		.name = "MDDMA_ASM4_M99"},
	{.master = MID_MDDMA_11, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x1807,
		.note = "MDDMA: PDTrace",
		.name = "MDDMA_PDTRACE_M99"},
	{.master = MID_MDDMA_12, .port = 0x4, .id_mask = 0x1FEF,
		.id_val = 0x180B,
		.note = "MDDMA: ULS",
		.name = "MDDMA_USL_M99"},
	{.master = MID_MDDMA_13, .port = 0x4, .id_mask = 0x1FFD,
		.id_val = 0x1800,
		.note = "MDDMA: MDINFRA_ASM",
		.name = "MDDMA_MDINFRA_ASM_M99"},
	{.master = MID_MDDMA_14, .port = 0x4, .id_mask = 0x1FFE,
		.id_val = 0x1A00,
		.note = "MDDMA: PS_PERI",
		.name = "MDDMA_PS_PERI_M99"},
	{.master = MID_MDDMA_15, .port = 0x4, .id_mask = 0x1FFE,
		.id_val = 0x1C00,
		.note = "MDDMA: LTEL2DMA",
		.name = "MDDMA_LTEL2DMA_M99"},
	{.master = MID_MDDMA_16, .port = 0x4, .id_mask = 0x1FFE,
		.id_val = 0x1E00,
		.note = "MDDMA: TRACE_TOP",
		.name = "MDDMA_TRACE_TOP_M99"},

	/* L1SYS */
	{.master = MID_L1SYS_0, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0882,
		.note = "L1SYS: hrq_rd",
		.name = "L1SYS_HRD_RD_M99"},
	{.master = MID_L1SYS_1, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0886,
		.note = "L1SYS: hrq_rd1",
		.name = "L1SYS_HRD_RD_M99"},
	{.master = MID_L1SYS_2, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0881,
		.note = "L1SYS: hrq_wr",
		.name = "L1SYS_HRD_WR_M99"},
	{.master = MID_L1SYS_3, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0885,
		.note = "L1SYS: hrq_wr1",
		.name = "L1SYS_HRD_WR_M99"},
	{.master = MID_L1SYS_4, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0880,
		.note = "L1SYS: sw_trc",
		.name = "L1SYS_SW_TRC_M99"},
	{.master = MID_L1SYS_5, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0884,
		.note = "L1SYS: hw_trc",
		.name = "L1SYS_HW_TRC_M99"},
	{.master = MID_L1SYS_6, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0888,
		.note = "L1SYS: vtb",
		.name = "L1SYS_VTB_M99"},
	{.master = MID_L1SYS_7, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x088C,
		.note = "L1SYS: tbo",
		.name = "L1SYS_TBO_M99"},
	{.master = MID_L1SYS_8, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0890,
		.note = "L1SYS: debug",
		.name = "L1SYS_DEBUG_M99"},
	{.master = MID_L1SYS_9, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x08C4,
		.note = "L1SYS: sw_trc",
		.name = "L1SYS_SW_TRC_M99"},
	{.master = MID_L1SYS_10, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x08C6,
		.note = "L1SYS: hw_trc",
		.name = "L1SYS_HW_TRC_M99"},
	{.master = MID_L1SYS_11, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x08C0,
		.note = "L1SYS: irdma",
		.name = "L1SYS_IRDMA_M99"},
	{.master = MID_L1SYS_12, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0801,
		.note = "L1SYS: txcal",
		.name = "L1SYS_TXCAL_M99"},
	{.master = MID_L1SYS_13, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0809,
		.note = "L1SYS: sw_trc",
		.name = "L1SYS_SW_TRC_M99"},
	{.master = MID_L1SYS_14, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0803,
		.note = "L1SYS: dbg0",
		.name = "L1SYS_DBG0_M99"},
	{.master = MID_L1SYS_15, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0805,
		.note = "L1SYS: dbg1",
		.name = "L1SYS_DBG1_M99"},
	{.master = MID_L1SYS_16, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0807,
		.note = "L1SYS: shm",
		.name = "L1SYS_SHM_M99"},
	{.master = MID_L1SYS_17, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0840,
		.note = "L1SYS: txcal",
		.name = "L1SYS_TXCAL_M99"},
	{.master = MID_L1SYS_18, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0844,
		.note = "L1SYS: sw_trc",
		.name = "L1SYS_SW_TRC_M99"},
	{.master = MID_L1SYS_19, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0841,
		.note = "L1SYS: dbg0",
		.name = "L1SYS_DBG0_M99"},
	{.master = MID_L1SYS_20, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0842,
		.note = "L1SYS: dbg1",
		.name = "L1SYS_DBG1_M99"},
	{.master = MID_L1SYS_21, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0843,
		.note = "L1SYS: shm",
		.name = "L1SYS_SHM_M99"},
	{.master = MID_L1SYS_22, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0800,
		.note = "L1SYS: cstxb_dcxo",
		.name = "L1SYS_CSTXB_DCXO_M99"},
	{.master = MID_L1SYS_23, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0802,
		.note = "L1SYS: cstxb_csh",
		.name = "L1SYS_CSTXB_CSH_M99"},
	{.master = MID_L1SYS_24, .port = 0x4, .id_mask = 0x1FFF,
		.id_val = 0x0804,
		.note = "L1SYS: cstxb_txbrp",
		.name = "L1SYS_CSTXB_TXBRP_M99"},

	/* MM CH2 */
	{.master = MID_MM_CH2_0, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0000,
		.note = "MM: LARB0",
		.name = "MM_LARB0_M99"},
	{.master = MID_MM_CH2_1, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0080,
		.note = "MM: LARB1",
		.name = "MM_LARB1_M99"},
	{.master = MID_MM_CH2_2, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0100,
		.note = "MM_VDEC: LARB4",
		.name = "MM_LARB4_M99"},
	{.master = MID_MM_CH2_3, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0180,
		.note = "MM_IMG: LARB5",
		.name = "MM_LARB5_M99"},
	{.master = MID_MM_CH2_4, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0200,
		.note = "MM_CAM: LARB6",
		.name = "MM_LARB6_M99"},
	{.master = MID_MM_CH2_5, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0300,
		.note = "MM_VENC: LARB7",
		.name = "MM_LARB7_M99"},
	{.master = MID_MM_CH2_6, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0280,
		.note = "MM_MJC: LARB8",
		.name = "MM_LARB8_M99"},
	{.master = MID_MM_CH2_7, .port = 0x5, .id_mask = 0x1F80,
		.id_val = 0x0380,
		.note = "MM_IPU: IPU",
		.name = "MM_IPU_M99"},
	{.master = MID_MM_CH2_8, .port = 0x5, .id_mask = 0x1FFF,
		.id_val = 0x07FC,
		.note = "MM_M4U: MM IOMMU Internal Used",
		.name = "MM_IOMMU_M99"},
	{.master = MID_MM_CH2_9, .port = 0x5, .id_mask = 0x1FFF,
		.id_val = 0x07FD,
		.note = "MM_M4U: MM IOMMU Internal Used",
		.name = "MM_IOMMU_M99"},

	/* GPU CH1 */
	{.master = MID_GPU_CH1_0, .port = 0x6, .id_mask = 0x1F80,
		.id_val = 0x0000,
		.note = "GPU: MFG_M0 and MFG_M1",
		.name = "GPU_MFG_M99"},

	/* GPU CH2 */
	{.master = MID_GPU_CH2_0, .port = 0x7, .id_mask = 0x1F80,
		.id_val = 0x0000,
		.note = "GPU: MFG_M0 and MFG_M1",
		.name = "GPU_MFG_M99"},

};

static const char *UNKNOWN_MASTER = "unknown";
static DEFINE_SPINLOCK(emi_mpu_lock);

#ifdef ENABLE_EMI_CHKER
struct timer_list emi_axi_vio_timer;
#endif

char *smi_larb0_port[8] = {
	"M99_LARB0_disp_ovl0", "M99_LARB0_disp_2L_ovl0", "M99_LARB0_disp_rdma0",
	"M99_LARB0_disp_wdma0", "M99_LARB0_mdp_rdma0", "M99_LARB0_mdp_wdma",
	"M99_LARB0_mdp_wrot0", "M99_LARB0_disp_fake0"};
char *smi_larb1_port[11] =  {
	"M99_LARB1_disp_ovl1", "M99_LARB1_disp_2L_ovl1", "M99_LARB1_disp_rdma1",
	"M99_LARB1_disp_rdma2", "M99_LARB1_disp_wdma1", "M99_LARB1_disp_od_r",
	"M99_LARB1_disp_od_w", "M99_LARB1_disp_2L_ovl0", "M99_LARB1_mdp_rdma1",
	"M99_LARB1_mdp_wrot1", "M99_LARB1_disp_fake1"};
char *smi_larb2_port[5] = {
	"M99_LARB2_ipuo", "M99_LARB2_ipu2o", "M99_LARB2_ipu3o",
	"M99_LARB2_ipui", "M99_LARB2_ipu2i"};
char *smi_larb3_port[5] = {
	"M99_LARB3_ipuo", "M99_LARB3_ipu2o", "M99_LARB3_ipu3o",
	"M99_LARB3_ipui", "M99_LARB3_ipu2i"};
char *smi_larb4_port[10] = {
	"M99_LARB4_vdec_mc_ext", "M99_LARB4_vdec_pp_ext", "M99_LARB4_vdec_ufo_ext",
	"M99_LARB4_vdec_vld_ext", "M99_LARB4_vdec_vld2_ext", "M99_LARB4_vdec_avc_mv_ext",
	"M99_LARB4_vdec_pred_rd_ext", "M99_LARB4_vdec_pred_wr_ext", "M99_LARB4_vdec_ppwrap_ext",
	"M99_LARB4_vdec_tile_ext"};
char *smi_larb5_port[21] = {
	"M99_LARB5_imgi_a", "M99_LARB5_img2o_a", "M99_LARB5_img3o_a",
	"M99_LARB5_vipi_a", "M99_LARB5_lcei_a", "M99_LARB5_wpe_a_rdma_0",
	"M99_LARB5_wpe_a_rdma_1", "M99_LARB5_wpe_a_wdma", "M99_LARB5_fd_dma_rp",
	"M99_LARB5_fd_dma_wr", "M99_LARB5_fd_dma_rb", "M99_LARB5_wpe_b_rdma_0",
	"M99_LARB5_wpe_b_rdma_1", "M99_LARB5_wpe_b_wdma", "M99_LARB5_eaf_rdma_0",
	"M99_LARB5_eaf_rdma_1", "M99_LARB5_eaf_wdma", "M99_LARB5_dpe_rdma",
	"M99_LARB5_dpe_wdma", "M99_LARB5_rsc_rdma0", "M99_LARB5_rsc_wdma"};
char *smi_larb6_port[19] = {
	"M99_LARB6_imgo", "M99_LARB6_rrzo", "M99_LARB6_aao",
	"M99_LARB6_afo", "M99_LARB6_lsci_0", "M99_LARB6_lsci_1",
	"M99_LARB6_pdo", "M99_LARB6_bpci", "M99_LARB6_lcso",
	"M99_LARB6_rsso", "M99_LARB6_ufeo", "M99_LARB6_soco",
	"M99_LARB6_soc1", "M99_LARB6_soc2", "M99_LARB6_ccui",
	"M99_LARB6_ccuo", "M99_LARB6_caci", "M99_LARB6_rawi",
	"M99_LARB6_ccug"};
char *smi_larb7_port[17] = {
	"M99_LARB7_venc_rcpu", "M99_LARB7_venc_rec", "M99_LARB7_venc_bsdma",
	"M99_LARB7_venc_sv_comv", "M99_LARB7_venc_sv_segid", "M99_LARB7_venc_rd_comv",
	"M99_LARB7_venc_rd_segid", "M99_LARB7_jpgenc_rdma", "M99_LARB7_jpgenc_bsdma",
	"M99_LARB7_jpgdec_wdma", "M99_LARB7_jpgdec_bsdma", "M99_LARB7_venc_cur_luma",
	"M99_LARB7_venc_cur_chroma", "M99_LARB7_venc_ref_luma", "M99_LARB7_venc_ref_chroma",
	"M99_LARB7_venc_nbm_rdma", "M99_LARB7_venc_nbm_wdma"};
char *smi_larb8_port[4] = {
	"M99_LARB8_mjc_mv_rd", "M99_LARB8_mjc_mv_wr", "M99_LARB8_mjc_dma_rd",
	"M99_LARB8_mjc_dma_wr"};

static int __match_id(u32 axi_id, int tbl_idx, u32 port_ID)
{
	u32 mm_larb;
	u32 smi_port;

	if (((axi_id & mst_tbl[tbl_idx].id_mask) == mst_tbl[tbl_idx].id_val)
		&& (port_ID == mst_tbl[tbl_idx].port)) {
		switch (port_ID) {
		case 0: /* MASTER_APMCU_CH1 */
		case 1: /* MASTER_APMCU_CH2 */
		case 3:	/* MASTER_MDMCU */
		case 4:	/* MASTER_MDDMA */
		case 6:	/* MASTER_GPU_CH1 */
		case 7:	/* MASTER_GPU_CH2 */
			pr_err("Violation master name is %s.\n",
			mst_tbl[tbl_idx].name);
			break;
		case 2: /* MASTER_MM_CH1 */
		case 5:	/* MASTER_MM_CH2 */
			mm_larb = axi_id >> 7;
			smi_port = (axi_id & 0x7F) >> 2;
			if (mm_larb == 0x0) {	/* smi_larb0 */
				if (smi_port >= ARRAY_SIZE(smi_larb0_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb0 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb0_port[smi_port]);
			} else if (mm_larb == 0x1) {	/* smi_larb1 */
				if (smi_port >= ARRAY_SIZE(smi_larb1_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb1 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb1_port[smi_port]);
			} else if (mm_larb == 0x2) {	/* smi_larb4 */
				if (smi_port >= ARRAY_SIZE(smi_larb4_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb4 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb4_port[smi_port]);
			} else if (mm_larb == 0x3) {	/* smi_larb5 */
				if (smi_port >= ARRAY_SIZE(smi_larb5_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb5 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb5_port[smi_port]);
			} else if (mm_larb == 0x4) {	/* smi_larb6 */
				if (smi_port >= ARRAY_SIZE(smi_larb6_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb6 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb6_port[smi_port]);
			} else if (mm_larb == 0x5) {	/* smi_larb8 */
				if (smi_port >= ARRAY_SIZE(smi_larb8_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb8 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb8_port[smi_port]);
			} else if (mm_larb == 0x6) {	/* smi_larb7 */
				if (smi_port >= ARRAY_SIZE(smi_larb7_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi larb7 failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb7_port[smi_port]);
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
			if ((port_ID == 2) || (port_ID == 5)) {
				mm_larbID = axi_ID >> 7;
				smi_portID = (axi_ID & 0x7F) >> 2;
				if (mm_larbID == 0x0)
					return smi_larb0_port[smi_portID];
				else if (mm_larbID == 0x1)
					return smi_larb1_port[smi_portID];
				else if (mm_larbID == 0x2)
					return smi_larb4_port[smi_portID];
				else if (mm_larbID == 0x3)
					return smi_larb5_port[smi_portID];
				else if (mm_larbID == 0x4)
					return smi_larb6_port[smi_portID];
				else if (mm_larbID == 0x5)
					return smi_larb8_port[smi_portID];
				else if (mm_larbID == 0x6)
					return smi_larb7_port[smi_portID];
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
	u32 dbg_s, dbg_t, dbg_t_2nd, dbg_pqry;
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
	dbg_pqry = readl(IOMEM(EMI_MPUD_ST(domain_ID)));

	/*TBD: print the abort region*/

	pr_err("EMI MPU violation.\n");
	pr_err("[EMI MPU] Debug info start ----------------------------------------\n");

	pr_err("EMI_MPUS = %x, EMI_MPUT = %x, EMI_MPUT_2ND = %x.\n", dbg_s, dbg_t, dbg_t_2nd);
	pr_err("Current process is \"%s \" (pid: %i).\n",
	current->comm, current->pid);
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
	/* EMI violation is relative to MD at user build*/
	#if 0 /* #ifndef CONFIG_MTK_ENG_BUILD */
		if (((master_ID & 0x7) == MASTER_MDMCU) ||
			((master_ID & 0x7) == MASTER_MDDMA)) {
			int md_id = 0;

			exec_ccci_kern_func_by_md_id(md_id,
			ID_FORCE_MD_ASSERT, NULL, 0);
			pr_err("[EMI MPU] MPU violation trigger MD\n");
		}
	#else
		if (((master_ID & 0x7) == MASTER_MDMCU) ||
			((master_ID & 0x8007) == (MASTER_MDDMA | 0x8000))) {
			char str[60] = "0";
			char *pstr = str;

			sprintf(pstr, "EMI_MPUS = 0x%x, ADDR = 0x%llx",
				dbg_s, vio_addr);

			exec_ccci_kern_func_by_md_id(0,
			ID_MD_MPU_ASSERT, str, strlen(str));
			pr_err("[EMI MPU] MPU violation trigger MD str=%s strlen(str)=%d\n"
				, str, (int)strlen(str));
		} else {
			exec_ccci_kern_func_by_md_id(0,
			ID_MD_MPU_ASSERT, NULL, 0);
			pr_err("[EMI MPU] MPU violation ack to MD\n");
		}
	#endif
		if ((region == 0) && (mt_emi_reg_read(EMI_MPU_SA(0)) == 0)
			&& (mt_emi_reg_read(EMI_MPU_EA(0)) == 0)
			&& (mt_emi_reg_read(EMI_MPU_APC(0, 0)) == 0)
			&& (!(dbg_pqry & OOR_VIO))) {
			pr_err("[EMI MPU] A strange violation.\n");
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
	int ch_nr = get_emi_channel_number();
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

	if (0 != ((emi_cona >> 8) & 0x02)) {

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

	if (pasrdpd->channel_nr == 4) {
		pasrdpd->channel[2] = pasrdpd->channel[1];
		pasrdpd->channel[3] = pasrdpd->channel[1];
		pasrdpd->channel[1] = pasrdpd->channel[0];
	} else {
		pasrdpd->channel[2].valid_ch = false;
		pasrdpd->channel[3].valid_ch = false;
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
		start = ((unsigned long long)mt_emi_reg_read(EMI_MPU_SA(region)) << 16) + emi_physical_offset;
		end = ((unsigned long long)mt_emi_reg_read(EMI_MPU_EA(region)) << 16) + emi_physical_offset;
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
	unsigned int access_permission;
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
		ret = kstrtoul(token[4], 16,
		(unsigned long *)&access_permission);
		if (ret != 0)
			pr_err("kstrtoul fails to parse access_permission");
		emi_mpu_set_region_protection(start_addr, end_addr,
		region, access_permission);
		pr_err("Set EMI_MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x.\n",
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

		pr_err("set EMI MPU: start: 0x%x, end: 0x%x, region: %d, permission: 0x%x\n",
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
					pr_err("[EMI MPU] Find page entry section at pte: %lx. violation address = 0x%llx\n",
					(unsigned long)(pte), v_addr);
					return;
				}
			}
		} else {
			/* Section */
			if (((unsigned long)pmd_val(*(pmd)) & SECTION_MASK) ==
				((unsigned long)v_addr
				& SECTION_MASK)) {
				pr_err("[EMI MPU] Find page entry section at pmd: %lx. violation address = 0x%llx\n",
				(unsigned long)(pmd), v_addr);
				return;
			}
		}
#else
		/* TBD */
#endif
	}
	pr_err("[EMI MPU] ****** Can not find page table entry! violation address = 0x%llx ******\n",
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
	dump_emi_latency();
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
static struct platform_driver emi_dump_latency_ctrl = {
	.driver = {
		.name = "emi_dump_latency_ctrl",
		.owner = THIS_MODULE,
	},
};
/*
 * con_sce_show: sysfs con_sce file show function.
 * @driver:
 * @buf:
 * Return the number of read bytes.
 */
static ssize_t dump_latency_ctrl_show(struct device_driver *driver, char *buf)
{
	char *ptr;

	ptr = (char *)buf;
	ptr += sprintf(ptr, "dump_latency_ctrl_show: is_dump_emi_latency: %d\n", is_dump_emi_latency);

	return strlen(buf);
}
static ssize_t dump_latency_ctrl_store(struct device_driver *driver,
const char *buf, size_t count)
{
	char *command;

	if ((strlen(buf) + 1) > MAX_EMI_MPU_STORE_CMD_LEN) {
		pr_err("dump_latency_ctrl_store command overflow.");
		return count;
	}
	pr_err("dump_latency_ctrl_store: %s\n", buf);

	command = kmalloc((size_t) MAX_EMI_MPU_STORE_CMD_LEN, GFP_KERNEL);
	if (!command)
		return count;

	strcpy(command, buf);

	if (!strncmp(buf, EN_MPU_STR, strlen(EN_MPU_STR)))
		is_dump_emi_latency = 1;
	else if (!strncmp(buf, DIS_MPU_STR, strlen(DIS_MPU_STR)))
		is_dump_emi_latency = 0;
	else
		pr_err("Unknown latency_ctrl command.\n");

	pr_err("dump_latency_ctrl_store: is_dump_emi_latency: %X\n", is_dump_emi_latency);

	kfree(command);

	return count;
}
DRIVER_ATTR(dump_latency_ctrl, 0644, dump_latency_ctrl_show, dump_latency_ctrl_store);


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

#if 1
#define AP_REGION_ID   31
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
unsigned int set_emi_mpu_intr_mask(unsigned int region, unsigned int domain)
{
	int val, mpu_do_rg_mask;

	mpu_do_rg_mask = mt_emi_reg_read(EMI_RG_MASK_D0 + (domain * 4));
	val = (mpu_do_rg_mask | (1 << region));
	mt_emi_reg_write(val, (EMI_RG_MASK_D0 + (domain * 4)));
	return 0;
}
int set_emi_mpu_region_intr_mask(unsigned int region, unsigned int domain)
{
	if ((region < EMI_MPU_REGION_NUMBER) || (domain < EMI_MPU_DOMAIN_NUMBER)) {
		set_emi_mpu_intr_mask(region, domain);
		pr_debug("mpu set mask (region,domain)=(%d,%d)\n", region, domain);
	} else {
		pr_err("mpu set mask overflow (region,domain)=(%d,%d)\n", region, domain);
		return -1;
	}
	return 0;
}
/*
 * emi_set_dcm_ratio : emi dcm ration should adjust
 * according to infra fmem dcm ratio
 */
int emi_set_dcm_ratio(unsigned int div_val)
{
	unsigned int dcm_ratio, conh, conh_2nd;
	int ret = 0;
/*
 * DCM_RATIO[4:0] = {EMI_CONH_2ND[2:0], EMI_CONH[1:0]}
 * DCM slow down ratio:
 * 5'b00000: no DCM
 * 5'b00001: 1/2 slow DCM
 * 5'd00011: 1/4 slow DCM
 * 5'd00111: 1/8 slow DCM
 * 5'd01111: 1/16 slow DCM
 * 5'd11111: 1/32 slow DCM
 */
	if (!((div_val == 1) || (div_val == 2) || (div_val == 4) ||
	(div_val == 8) || (div_val == 16) || (div_val == 32))) {
		pr_err("emi_set_dcm_ratio div_val ERROR =[%d]", div_val);
		return -1;
	}
	dcm_ratio = div_val - 1;
	conh = readl(IOMEM(EMI_CONH));
	conh_2nd = readl(IOMEM(EMI_CONH_2ND));
	conh = (conh & ~0x3)|(dcm_ratio & 0x3);
	conh_2nd = (conh_2nd & ~0x7) | ((dcm_ratio >> 2) & 0x7);

	mt_reg_sync_writel(conh, EMI_CONH);
	mt_reg_sync_writel(conh_2nd, EMI_CONH_2ND);
	return ret;
}
/* get EMI channel number */
unsigned int get_emi_channel_number(void)
{
	unsigned int val, emi_cona, ch_nr;

	emi_cona = readl(IOMEM(EMI_CONA));
	val = (emi_cona >> 8) & 0x3;

	switch (val) {
	case 0:
		ch_nr = 1;
		break;
	case 1:
		ch_nr = 2;
		break;
	case 2:
		ch_nr = 4;
		break;
	case 3:
		ch_nr = 0;
		break;
	}
	return ch_nr;
}
/*
 * lpdma_emi_ch23_get_status :for ch23 status
 *
 * @return 1 ch2,3 is idle
 * @return 0 ch2,3 is working
 */
int lpdma_emi_ch23_get_status(void)
{
	unsigned int val, emi_sta;

	/*0x1000_0254[3:0] : 4CH idlearegister*/
	emi_sta = readl(IOMEM(EMI_CH_STA));
	val = (emi_sta >> 2) & 0x3;
	if (val == 0x3)
		return 1;
	else
		return 0;
}

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
	pr_err("[EMI_MPU]   ptr0_phyical : %p\n", ptr0_phyical);
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
			pr_err("get EMI_MPU irq = %d\n", mpu_irq);
			pr_err("get EMI_BASE_ADDR @ %p\n", EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (CHN0_EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,chn0_emi");
		if (node) {
			CHN0_EMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get CHN0_EMI_BASE_ADDR @ %p\n", CHN0_EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (CHN1_EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,chn1_emi");
		if (node) {
			CHN1_EMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get CHN0_EMI_BASE_ADDR @ %p\n", CHN1_EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (CHN2_EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,chn2_emi");
		if (node) {
			CHN2_EMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get CHN0_EMI_BASE_ADDR @ %p\n", CHN2_EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (CHN3_EMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,chn3_emi");
		if (node) {
			CHN3_EMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get CHN0_EMI_BASE_ADDR @ %p\n", CHN3_EMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}

	if (INFRA_CFG == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
		if (node) {
			INFRA_CFG = of_iomap(node, 0);
			pr_err("get  INFRA_CFG@ %p\n", INFRA_CFG);
		} else {
			pr_err("can't find compatible node\n");
			return -1;
		}
	}
	if (INFRA_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-infracfg_ao");
		if (node) {
			INFRA_BASE_ADDR = of_iomap(node, 0);
			pr_err("get INFRA_BASE_ADDR @ %p\n", INFRA_BASE_ADDR);
		} else {
			pr_err("can't find compatible node INFRA_BASE_ADDR\n");
			return -1;
		}
	}
	pr_err("[EMI MPU] EMI_MPUS = 0x%x\n", readl(IOMEM(EMI_MPUS)));
	pr_err("[EMI MPU] EMI_MPUT = 0x%x\n", readl(IOMEM(EMI_MPUT)));
	pr_err("[EMI MPU] EMI_MPUT_2ND = 0x%x\n", readl(IOMEM(EMI_MPUT_2ND)));

	pr_err("[EMI MPU] EMI_WP_ADR = 0x%x\n", readl(IOMEM(EMI_WP_ADR)));
	pr_err("[EMI MPU] EMI_WP_ADR_2ND = 0x%x\n", readl(IOMEM(EMI_WP_ADR_2ND)));
	pr_err("[EMI MPU] EMI_WP_CTRL = 0x%x\n", readl(IOMEM(EMI_WP_CTRL)));
	pr_err("[EMI MPU] EMI_CHKER = 0x%x\n", readl(IOMEM(EMI_CHKER)));
	pr_err("[EMI MPU] EMI_CHKER_TYPE = 0x%x\n",
	readl(IOMEM(EMI_CHKER_TYPE)));
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
	if (mpu_irq != 0) {
		ret = request_irq(mpu_irq,
		(irq_handler_t)mpu_violation_irq, IRQF_TRIGGER_LOW | IRQF_SHARED,
		"mt_emi_mpu", &emi_mpu_ctrl);
		if (ret != 0) {
			pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
			return ret;
		}
	}

#if 1
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

	/* register emi_dump_latency_ctrl interface */
	is_dump_emi_latency = 1;
	ret = platform_driver_register(&emi_dump_latency_ctrl);
	if (ret)
		pr_err("[EMI/BWM] fail to register emi_dump_latency_ctrl driver\n");

	ret = driver_create_file(&emi_dump_latency_ctrl.driver, &driver_attr_dump_latency_ctrl);
	if (ret)
		pr_err("[EMI/BWM] fail to create emi_dump_latency_ctrl sysfs file\n");

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
#define EMI_BCNT                     IOMEM(EMI_BASE_ADDR+0x408)
#define EMI_WSCT                     IOMEM(EMI_BASE_ADDR+0x428)
#define EMI_WSCT2                    IOMEM(EMI_BASE_ADDR+0x458)
#define EMI_WSCT3                    IOMEM(EMI_BASE_ADDR+0x460)
#define EMI_WSCT4                    IOMEM(EMI_BASE_ADDR+0x464)
#define EMI_TTYPE1                   IOMEM(EMI_BASE_ADDR+0x500)
#define EMI_TTYPE2                   IOMEM(EMI_BASE_ADDR+0x508)
#define EMI_TTYPE3                   IOMEM(EMI_BASE_ADDR+0x510)
#define EMI_TTYPE4                   IOMEM(EMI_BASE_ADDR+0x518)
#define EMI_TTYPE5                   IOMEM(EMI_BASE_ADDR+0x520)
#define EMI_TTYPE6                   IOMEM(EMI_BASE_ADDR+0x528)
#define EMI_TTYPE7                   IOMEM(EMI_BASE_ADDR+0x530)
#define EMI_TTYPE8                   IOMEM(EMI_BASE_ADDR+0x538)
#define EMI_TTYPE9                   IOMEM(EMI_BASE_ADDR+0x540)
#define EMI_TTYPE10                  IOMEM(EMI_BASE_ADDR+0x548)
#define EMI_TTYPE11                  IOMEM(EMI_BASE_ADDR+0x550)
#define EMI_TTYPE12                  IOMEM(EMI_BASE_ADDR+0x558)
#define EMI_TTYPE13                  IOMEM(EMI_BASE_ADDR+0x560)
#define EMI_TTYPE14                  IOMEM(EMI_BASE_ADDR+0x568)
#define EMI_TTYPE15                  IOMEM(EMI_BASE_ADDR+0x570)
#define EMI_TTYPE16                  IOMEM(EMI_BASE_ADDR+0x578)
#define EMI_APBDBG                   IOMEM(EMI_BASE_ADDR+0x7FC)

#define CHN0_EMI_CONB                IOMEM(CHN0_EMI_BASE_ADDR+0x008)
#define CHN0_EMI_DBGA                IOMEM(CHN0_EMI_BASE_ADDR+0xA80)
#define CHN0_EMI_DBGB                IOMEM(CHN0_EMI_BASE_ADDR+0xA84)
#define CHN0_EMI_DBGC                IOMEM(CHN0_EMI_BASE_ADDR+0xA88)
#define CHN0_EMI_DBGD                IOMEM(CHN0_EMI_BASE_ADDR+0xA8C)
#define CHN1_EMI_CONB                IOMEM(CHN1_EMI_BASE_ADDR+0x008)
#define CHN1_EMI_DBGA                IOMEM(CHN1_EMI_BASE_ADDR+0xA80)
#define CHN1_EMI_DBGB                IOMEM(CHN1_EMI_BASE_ADDR+0xA84)
#define CHN1_EMI_DBGC                IOMEM(CHN1_EMI_BASE_ADDR+0xA88)
#define CHN1_EMI_DBGD                IOMEM(CHN1_EMI_BASE_ADDR+0xA8C)

#include <mtk_dramc.h>

static inline void aee_simple_print(const char *msg, unsigned int val)
{
	char buf[128];

	snprintf(buf, sizeof(buf), msg, val);
	aee_sram_fiq_log(buf);
}

#define EMI_DBG_SIMPLE_RWR(msg, addr, wval)	do {\
	aee_simple_print(msg, readl(addr));	\
	writel(wval, addr);			\
	aee_simple_print(msg, readl(addr));\
	} while (0)

#define EMI_DBG_SIMPLE_R(msg, addr)		\
	aee_simple_print(msg, readl(addr))

/* 0x10230000 - 0x10230fff emi, EMI_BASE_ADDR */
/* 0x10335000 - 0x10335fff chn0_emi, CHN0_EMI_BASE_ADDR */
/* 0x1033d000 - 0x1033dfff chn1_emi, CHN1_EMI_BASE_ADDR */
/* 0x10345000 - 0x10345fff chn2_emi, CHN2_EMI_BASE_ADDR */
/* 0x1034d000 - 0x1034dfff chn3_emi, CHN3_EMI_BASE_ADDR */
/* 0x10260000 - 0x10260fff infra_cfg, INFRA_CFG */

void dump_emi_outstanding(void)
{
	if (!EMI_BASE_ADDR)
		return;

	if (!CHN0_EMI_BASE_ADDR)
		return;

	if (!INFRA_CFG)
		return;

	EMI_DBG_SIMPLE_RWR("[EMI] 0x102300e8 = 0x%x\n",
				EMI_BASE_ADDR+0xe8, 0x00060121);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10355a80 = 0x%x\n",
				CHN0_EMI_BASE_ADDR+0xa80, 0x00000001);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00000100);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00000200);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00000300);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00000400);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n",
				EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n",
				EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00000500);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00000600);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00000700);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00000400);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00000800);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00000900);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00001200);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00001100);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00001500);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00001600);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00001700);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00001800);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00001900);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00001a00);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00001b00);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00001c00);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00003600);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00003700);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00003800);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00003900);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00002a00);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00002b00);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00002c00);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00002d00);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00002600);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00002700);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00002800);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00002900);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00002200);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00002300);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00002400);
	EMI_DBG_SIMPLE_RWR("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00002500);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	writel(0x1, CHN0_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x1, CHN1_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x1, CHN2_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x1, CHN3_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
}

#define EMI_DBG_SIMPLE_RWR_MD(msg, addr, wval)	do {\
	pr_err(msg, readl(addr));	\
	writel(wval, addr);			\
	pr_err(msg, readl(addr));\
	} while (0)

#define EMI_DBG_SIMPLE_R_MD(msg, addr)		\
	pr_err(msg, readl(addr))

void dump_emi_outstanding_for_md(void)
{
	if (!EMI_BASE_ADDR)
		return;

	if (!CHN0_EMI_BASE_ADDR)
		return;

	if (!INFRA_CFG)
		return;

	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x102300e8 = 0x%x\n",
				EMI_BASE_ADDR+0xe8, 0x00060121);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10355a80 = 0x%x\n",
				CHN0_EMI_BASE_ADDR+0xa80, 0x00000001);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00000100);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00000200);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00000300);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00000400);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n",
				EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n",
				EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00000500);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00000600);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00000700);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00000400);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00000800);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00000900);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00001200);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00001100);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00001500);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00001600);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00001700);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00001800);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00001900);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00001a00);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00001b00);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00001c00);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00003600);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00003700);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00003800);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00003900);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00002a00);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00002b00);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00002c00);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00002d00);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00002600);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00002700);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00002800);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00002900);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260100 = 0x%x\n",
				INFRA_CFG+0x100, 0x00002200);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260104 = 0x%x\n",
				INFRA_CFG+0x104, 0x00002300);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x10260108 = 0x%x\n",
				INFRA_CFG+0x108, 0x00002400);
	EMI_DBG_SIMPLE_RWR_MD("[EMI] 0x1026010c = 0x%x\n",
				INFRA_CFG+0x10c, 0x00002500);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x102307fc = 0x%x\n", EMI_BASE_ADDR+0x7fc);
	writel(0x1, CHN0_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN0_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN0_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10335a84 = 0x%x\n", CHN0_EMI_BASE_ADDR+0xa84);
	writel(0x1, CHN1_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN1_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN1_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1033da84 = 0x%x\n", CHN1_EMI_BASE_ADDR+0xa84);
	writel(0x1, CHN2_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN2_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN2_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x10345a84 = 0x%x\n", CHN2_EMI_BASE_ADDR+0xa84);
	writel(0x1, CHN3_EMI_BASE_ADDR+0xa80);
	writel(0x160015, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x180017, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0x1a0019, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x1001b, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0x230022, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x250024, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0x50004, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x70006, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0xe0008, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0x10000f, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	writel(0xa0009, CHN3_EMI_BASE_ADDR+0xa88);
	writel(0xc000b, CHN3_EMI_BASE_ADDR+0xa8c);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
	EMI_DBG_SIMPLE_R_MD("[EMI] 0x1034da84 = 0x%x\n", CHN3_EMI_BASE_ADDR+0xa84);
}

void dump_emi_latency(void)
{
	unsigned int temp;
	unsigned int bw_total, bw_apmcu, bw_mdmcu, bw_mm;
	unsigned int cycle_cnt;
	unsigned int lat_m0_apmcu0, lat_m1_apmcu1;
	unsigned int lat_m5_mm0, lat_m2_mm1;
	unsigned int lat_m3_mdmcu, lat_m4_mdhw;
	unsigned int lat_m6_peri, lat_m7_gpu;
	unsigned int cnt_m0_apmcu0, cnt_m1_apmcu1;
	unsigned int cnt_m5_mm0, cnt_m2_mm1;
	unsigned int cnt_m3_mdmcu, cnt_m4_mdhw;
	unsigned int cnt_m6_peri, cnt_m7_gpu;
	unsigned int value_bmrw[2];
	unsigned int i;

	pr_err("[EMI latency] period 5 ms\n");
	pr_err("[EMI latency] DRAM data rate: %d\n", get_dram_data_rate());

	if (!(is_dump_emi_latency)) {
		pr_err("met is running ,should not dump emi latency\n");
		return;
	}
	pr_err("[EMI latency] starting...\n");
	value_bmrw[0] = 0x55555555;
	value_bmrw[1] = 0xaaaaaaaa;

	for (i = 0; i < 2; i++) {

		/* Clear bus monitor */
		temp = readl(EMI_BMEN) & 0xfffffffc;
		writel(temp, EMI_BMEN);

		/* Setup EMI bus monitor:
		 * 1. Counter for {trans, word, defined type} = {TSCT, WSCT, TTYPE}
		 * 2. R/W command monitor depends on (global) BMEN[5:4] and (local) BMRW0
		 */
		writel(0x00ff0000, EMI_BMEN); /* Reset counter and set WSCT for ALL */
		writel(0x00240003, EMI_MSEL);
		writel(0x00000018, EMI_MSEL2);
		writel(0x02000000, EMI_BMEN2);
		writel(value_bmrw[i], EMI_BMRW0);
		pr_err("[EMI latency] EMI_BMRW0: 0x%x\n", value_bmrw[i]);

		/* Enable bus monitor */
		temp = readl(EMI_BMEN);
		writel(temp | 0x1, EMI_BMEN);

		mdelay(5);

		/* Pause bus monitor */
		temp = readl(EMI_BMEN);
		writel(temp | 0x2, EMI_BMEN);

		/* Get BW result */
		cycle_cnt = readl(EMI_BCNT); /* frequency = dram data rate /4 */
		bw_total = readl(EMI_WSCT); /* Unit: 8 bytes */
		bw_apmcu = readl(EMI_WSCT2);
		bw_mdmcu = readl(EMI_WSCT4);
		bw_mm = readl(EMI_WSCT3);

		pr_err("[EMI latency] Cycle count (EMI_BCNT) %d\n", cycle_cnt);
		pr_err("[EMI latency] Total BW (EMI_WSCT) %d\n", bw_total);
		pr_err("[EMI latency] APMCU BW (EMI_WSCT2) %d\n", bw_apmcu);
		pr_err("[EMI latency] MDMCU BW (EMI_WSCT4) %d\n", bw_mdmcu);
		pr_err("[EMI latency] MM BW (EMI_WSCT3) %d\n", bw_mm);

		/* Get latency result */
		lat_m0_apmcu0 = readl(EMI_TTYPE1);
		lat_m1_apmcu1 = readl(EMI_TTYPE2);
		lat_m2_mm1 = readl(EMI_TTYPE3);
		lat_m3_mdmcu = readl(EMI_TTYPE4);
		lat_m4_mdhw = readl(EMI_TTYPE5);
		lat_m5_mm0 = readl(EMI_TTYPE6);
		lat_m6_peri = readl(EMI_TTYPE7);
		lat_m7_gpu = readl(EMI_TTYPE8);

		cnt_m0_apmcu0 = readl(EMI_TTYPE9);
		cnt_m1_apmcu1 = readl(EMI_TTYPE10);
		cnt_m2_mm1 = readl(EMI_TTYPE11);
		cnt_m3_mdmcu = readl(EMI_TTYPE12);
		cnt_m4_mdhw = readl(EMI_TTYPE13);
		cnt_m5_mm0 = readl(EMI_TTYPE14);
		cnt_m6_peri = readl(EMI_TTYPE15);
		cnt_m7_gpu = readl(EMI_TTYPE16);

		pr_err("[EMI latency] (0)APMCU0 %d cycles for %d trans\n", lat_m0_apmcu0, cnt_m0_apmcu0);
		pr_err("[EMI latency] (1)APMCU1 %d cycles for %d trans\n", lat_m1_apmcu1, cnt_m1_apmcu1);
		pr_err("[EMI latency] (5)MM0 %d cycles for %d trans\n", lat_m5_mm0, cnt_m5_mm0);
		pr_err("[EMI latency] (2)MM1 %d cycles for %d trans\n", lat_m2_mm1, cnt_m2_mm1);
		pr_err("[EMI latency] (3)MDMCU %d cycles for %d trans\n", lat_m3_mdmcu, cnt_m3_mdmcu);
		pr_err("[EMI latency] (4)MDHW %d cycles for %d trans\n", lat_m4_mdhw, cnt_m4_mdhw);
		pr_err("[EMI latency] (6)PERI %d cycles for %d trans\n", lat_m6_peri, cnt_m6_peri);
		pr_err("[EMI latency] (7)GPU %d cycles for %d trans\n", lat_m7_gpu, cnt_m7_gpu);
	}

	dump_emi_outstanding_for_md();
}

