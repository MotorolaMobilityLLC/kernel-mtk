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
#include <mt-plat/mt_lpae.h>
#include <mt-plat/mt_ccci_common.h>
#include <linux/delay.h>

static int Violation_PortID = MASTER_ALL;


#define ENABLE_EMI_CHKER
#define ENABLE_EMI_WATCH_POINT

#define MAX_EMI_MPU_STORE_CMD_LEN 128
#define TIMEOUT 100
#define AXI_VIO_MONITOR_TIME	(1 * HZ)

static struct work_struct emi_mpu_work;
static struct workqueue_struct *emi_mpu_workqueue;
void __iomem *INFRA_BASE_ADDR = NULL;
void __iomem *INFRACFG_BASE_ADDR = NULL;
void __iomem *SMI_BASE_ADDR = NULL;
static unsigned int enable_4gb;
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
	/* apmcu ch1 */
	{ .master = MST_ID_APMCU_CH1_0,  .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x4,	.note = "Core nn system domain store exclusive",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_1,  .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x44, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_2,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x84, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_3,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x94,	.note = "SCU generated barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_4,  .port = 0x0, .id_mask = 0x1FEF,
		.id_val = 0xA4,	.note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_5,  .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0xC4,	.note = "Core nn non-re-orderable device write",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_6,  .port = 0x0, .id_mask = 0x1F0F,
		.id_val = 0x104,
		.note = "Write to normal memory, or re-orderable device memory",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_7,  .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x4,
		.note = "Core nn exclusive read or non-reorderable device read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_8,  .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x44, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_9,  .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x84, .note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_10, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x94,	.note = "SCU generated barrier or DVM complete",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_11, .port = 0x0, .id_mask = 0x1FEF,
		.id_val = 0xA4, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_12, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0xC4, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_13, .port = 0x0, .id_mask = 0x1F3F,
		.id_val = 0x104,	.note = "ACP read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_14, .port = 0x0, .id_mask = 0x1F3F,
		.id_val = 0x114,	.note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_15, .port = 0x0, .id_mask = 0x1F2F,
		.id_val = 0x124,	.note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_16, .port = 0x0, .id_mask = 0x1E0F,
		.id_val = 0x204, .note = "Core nn read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_17, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x3,	.note = "Core nn system domain store exclusive",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_18, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x43, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_19, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x83, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_20, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x93,	.note = "SCU generated barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_21, .port = 0x0, .id_mask = 0x1FEF,
		.id_val = 0x93, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_22, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0xC3, .note = "Core nn non-re-orderable device write",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_23, .port = 0x0, .id_mask = 0x1F0F,
		.id_val = 0x103,
		.note = "Write to normal memory, or re-orderable device memory",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_24, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x3,
		.note = "Core nn exclusive read or non-reorderable device read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_25, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x43,	.note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_26, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x83, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_27, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x93,	.note = "SCU generated barrier or DVM complete",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_28, .port = 0x0, .id_mask = 0x1FEF,
		.id_val = 0xA3, .note = "Unused",	.name = "CA53_m_97"},
	{ .master = MST_ID_APMCU_CH1_29, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0xC3, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_30, .port = 0x0, .id_mask = 0x1F3F,
		.id_val = 0x103,	.note = "ACP read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_31, .port = 0x0, .id_mask = 0x1F3F,
		.id_val = 0x113, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_32, .port = 0x0, .id_mask = 0x1F2F,
		.id_val = 0x123, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_33, .port = 0x0, .id_mask = 0x1E0F,
		.id_val = 0x203,	.note = "Core nn read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_34, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x5,	.note = "Core nn system domain store exclusive",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_35, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x45,	.note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_36, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x85, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_37, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x95,	.note = "SCU generated barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_38, .port = 0x0, .id_mask = 0x1FEF,
		.id_val = 0xA5, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_39, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0xC5, .note = "Core nn non-re-orderable device write",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_40, .port = 0x0, .id_mask = 0x1F0F,
		.id_val = 0x105,
		.note = "Write to normal memory, or re-orderable device memory",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_41, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x5,
		.note = "Core nn exclusive read or non-reorderable device read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_42, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0x45, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_43, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x85,	.note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_44, .port = 0x0, .id_mask = 0x1FFF,
		.id_val = 0x95,	.note = "SCU generated barrier or DVM complete",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_45, .port = 0x0, .id_mask = 0x1FEF,
		.id_val = 0xA5,	.note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_46, .port = 0x0, .id_mask = 0x1FCF,
		.id_val = 0xC5, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_47, .port = 0x0, .id_mask = 0x1F3F,
		.id_val = 0x105,	.note = "ACP read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_48, .port = 0x0, .id_mask = 0x1F3F,
		.id_val = 0x115, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_49, .port = 0x0, .id_mask = 0x1F2F,
		.id_val = 0x125, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH1_50, .port = 0x0, .id_mask = 0x1E0F,
		.id_val = 0x205,	.note = "Core nn read",
		.name = "CA53_m_97" },

	/* apmcu ch2 */
	{ .master = MST_ID_APMCU_CH2_0,  .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x4,	.note = "Core nn system domain store exclusive",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_1,  .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x44, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_2,  .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x84, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_3,  .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x94,	.note = "SCU generated barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_4,  .port = 0x1, .id_mask = 0x1FEF,
		.id_val = 0xA4,	.note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_5,  .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0xC4,	.note = "Core nn non-re-orderable device write",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_6,  .port = 0x1, .id_mask = 0x1F0F,
		.id_val = 0x104,
		.note = "Write to normal memory, or re-orderable device memory",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_7,  .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x4,
		.note = "Core nn exclusive read or non-reorderable device read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_8,  .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x44, .note = "Core nn barrier",
		.name = "CA53_m_97"},
	{ .master = MST_ID_APMCU_CH2_9,  .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x84, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_10, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x94,	.note = "SCU generated barrier or DVM complete",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_11, .port = 0x1, .id_mask = 0x1FEF,
		.id_val = 0xA4, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_12, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0xC4, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_13, .port = 0x1, .id_mask = 0x1F3F,
		.id_val = 0x104,	.note = "ACP read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_14, .port = 0x1, .id_mask = 0x1F3F,
		.id_val = 0x114,	.note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_15, .port = 0x1, .id_mask = 0x1F2F,
		.id_val = 0x124,	.note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_16, .port = 0x1, .id_mask = 0x1E0F,
		.id_val = 0x204, .note = "Core nn read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_17, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x3,	.note = "Core nn system domain store exclusive",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_18, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x43, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_19, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x83, .note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_20, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x93,	.note = "SCU generated barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_21, .port = 0x1, .id_mask = 0x1FEF,
		.id_val = 0x93, .note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_22, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0xC3, .note = "Core nn non-re-orderable device write",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_23, .port = 0x1, .id_mask = 0x1F0F,
		.id_val = 0x103,
		.note = "Write to normal memory, or re-orderable device memory",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_24, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x3,
		.note = "Core nn exclusive read or non-reorderable device read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_25, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x43,	.note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_26, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x83, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_27, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x93,	.note = "SCU generated barrier or DVM complete",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_28, .port = 0x1, .id_mask = 0x1FEF,
		.id_val = 0xA3, .note = "Unused",	.name = "CA53_m_97"},
	{ .master = MST_ID_APMCU_CH2_29, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0xC3, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_30, .port = 0x1, .id_mask = 0x1F3F,
		.id_val = 0x103,	.note = "ACP read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_31, .port = 0x1, .id_mask = 0x1F3F,
		.id_val = 0x113, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_32, .port = 0x1, .id_mask = 0x1F2F,
		.id_val = 0x123, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_33, .port = 0x1, .id_mask = 0x1E0F,
		.id_val = 0x203,	.note = "Core nn read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_34, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x5,	.note = "Core nn system domain store exclusive",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_35, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x45,	.note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_36, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x85, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_37, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x95,	.note = "SCU generated barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_38, .port = 0x1, .id_mask = 0x1FEF,
		.id_val = 0xA5, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_39, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0xC5,
		.note = "Core nn non-re-orderable device write",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_40, .port = 0x1, .id_mask = 0x1F0F,
		.id_val = 0x105,
		.note = "Write to normal memory, or re-orderable device memory",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_41, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x5,
		.note = "Core nn exclusive read or non-reorderable device read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_42, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0x45, .note = "Core nn barrier",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_43, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x85,	.note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_44, .port = 0x1, .id_mask = 0x1FFF,
		.id_val = 0x95,	.note = "SCU generated barrier or DVM complete",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_45, .port = 0x1, .id_mask = 0x1FEF,
		.id_val = 0xA5,	.note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_46, .port = 0x1, .id_mask = 0x1FCF,
		.id_val = 0xC5, .note = "Unused",	.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_47, .port = 0x1, .id_mask = 0x1F3F,
		.id_val = 0x105,	.note = "ACP read",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_48, .port = 0x1, .id_mask = 0x1F3F,
		.id_val = 0x115, .note = "Unused", .name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_49, .port = 0x1, .id_mask = 0x1F2F,
		.id_val = 0x125, .note = "Unused",
		.name = "CA53_m_97" },
	{ .master = MST_ID_APMCU_CH2_50, .port = 0x1, .id_mask = 0x1E0F,
		.id_val = 0x205,	.note = "Core nn read",
		.name = "CA53_m_97" },

/* MM ch1 */
{ .master = MST_ID_MM_CH1_0, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x0, .note = "smi_larb0", .name = "smi_larb0_m_97" },
{ .master = MST_ID_MM_CH1_1, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x80, .note = "smi_larb1", .name = "smi_larb1_m_97" },
{ .master = MST_ID_MM_CH1_2, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x100, .note = "smi_larb2", .name = "smi_larb2_m_97" },
{ .master = MST_ID_MM_CH1_3, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x180, .note = "smi_larb3", .name = "smi_larb3_m_97" },
{ .master = MST_ID_MM_CH1_4, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x200, .note = "smi_larb4", .name = "smi_larb4_m_97" },
{ .master = MST_ID_MM_CH1_5, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x280, .note = "smi_larb5", .name = "smi_larb5_m_97" },
{ .master = MST_ID_MM_CH1_6, .port = 0x2, .id_mask = 0x1F80,
	.id_val = 0x300, .note = "smi_larb6", .name = "smi_larb6_m_97" },
{ .master = MST_ID_MM_CH1_7, .port = 0x2, .id_mask = 0x1FFF,
	.id_val = 0x3FC, .note = "MM IOMMU Internal Used",
	.name = "MM_IOMMU_Internal_Used_m_97" },
{ .master = MST_ID_MM_CH1_8, .port = 0x2, .id_mask = 0x1FFF,
	.id_val = 0x3FD, .note = "MM IOMMU Internal Used",
	.name = "MM_IOMMU_Internal_Used_m_97" },

/* Modem MCU*/
{ .master = MST_ID_MDMCU_0,    .port = 0x3, .id_mask = 0x161F,
	.id_val = 0x0, .note = "MDMCU arm7", .name = "MDMCU_m_97" },
{ .master = MST_ID_MDMCU_1,    .port = 0x3, .id_mask = 0x1FFF,
	.id_val = 0x4, .note = "MDMCU arm7", .name = "MDMCU_m_97" },
{ .master = MST_ID_MDMCU_2,    .port = 0x3, .id_mask = 0x161F,
	.id_val = 0x8, .note = "MDMCU PREFETCH", .name = "MDMCU_m_97" },
{ .master = MST_ID_MDMCU_3,    .port = 0x3, .id_mask = 0x161F,
	.id_val = 0x10, .note = "MDMCU ALC", .name = "MDMCU_m_97" },
{ .master = MST_ID_MD_L1MCU_0, .port = 0x3, .id_mask = 0x1FFF,
	.id_val = 0x1, .note = "L1MCU", .name = "L1MCU_m_97" },
{ .master = MST_ID_C2KMCU_0,   .port = 0x3, .id_mask = 0x1FFF,
	.id_val = 0x6, .note = "L1MCU ARM D port", .name = "C2KMCU_m_97" },
{ .master = MST_ID_C2KMCU_1,   .port = 0x3, .id_mask = 0x1FFF,
	.id_val = 0x12, .note = "C2KMCU ARM I port", .name = "C2KMCU_m_97" },
{ .master = MST_ID_C2KMCU_2,   .port = 0x3, .id_mask = 0x1FFF,
	.id_val = 0x26, .note = "C2KMCU DMA port", .name = "C2KMCU_m_97" },

/* Modem HW (2G/3G/LTE) */
{ .master = MST_ID_MDHW_0,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x0, .note = "AP2MDREG", .name = "MDHWMIX_AP2MDREG_m_97" },
{ .master = MST_ID_MDHW_1,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x1, .note = "MDDBGSYS", .name = "MDHWMIX_MDDBGSYS_m_97" },
{ .master = MST_ID_MDHW_2,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x2, .note = "PSMCU2REG",
	.name = "MDHWMIX_PSMCU2REG_m_97" },
{ .master = MST_ID_MDHW_3,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x100, .note = "MDGDMA", .name = "MDHWMIX_MDGDMA_m_97" },
{ .master = MST_ID_MDHW_4,   .port = 0x4,  .id_mask = 0x1FFE,
	.id_val = 0x208, .note = "HSPAL2", .name = "MDHWMIX_HSPAL2_m_97" },
{ .master = MST_ID_MDHW_5,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x3C0, .note = "Rake_MD2G",
	.name = "MDHWMIX_Rake_MD2G_m_97" },
{ .master = MST_ID_MDHW_6,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x340, .note = "DFE_B4", .name = "MDHWMIX_DFE_B4_m_97" },
{ .master = MST_ID_MDHW_7,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x341, .note = "DFE_DBG", .name = "MDHWMIX_DFE_DBG_m_97" },
{ .master = MST_ID_MDHW_8,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x342, .note = "DFE_DBG1", .name = "MDHWMIX_DFE_DBG1_m_97" },
{ .master = MST_ID_MDHW_9,   .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x343, .note = "DFE_SHM", .name = "MDHWMIX_DFE_SHM_m_97" },
{ .master = MST_ID_MDHW_10,  .port = 0x4,  .id_mask = 0x1FFF,
	.id_val = 0x380, .note = "RXBRP_HRQ_WR",
	.name = "MDHWMIX_RXBRP_HRQ_WR_m_97" },
{ .master = MST_ID_MDHW_11,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x384, .note = "RXBRP_HRQ_WR1",
	.name = "MDHWMIX_RXBRP_HRQ_WR1_m_97" },
{ .master = MST_ID_MDHW_12,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x381, .note = "RXBRP_HRQ_RD",
	.name = "MDHWMIX_RXBRP_HRQ_RD_m_97" },
{ .master = MST_ID_MDHW_13,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x385, .note = "RXBRP_HRQ_RD1",
	.name = "MDHWMIX_RXBRP_HRQ_RD1_m_97" },
{ .master = MST_ID_MDHW_14,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x382, .note = "RXBRP_HRQ_DBG",
	.name = "MDHWMIX_RXBRP_HRQ_DBG_m_97" },
{ .master = MST_ID_MDHW_15,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x386, .note = "RXBRP_HRQ_OTH",
	.name = "MDHWMIX_RXBRP_HRQ_OTH_m_97" },
{ .master = MST_ID_MDHW_16,  .port = 0x4, .id_mask = 0x1FC7,
	.id_val = 0x300, .note = "L1MCU", .name = "MDHWMIX_L1MCU_m_97" },
{ .master = MST_ID_MDHW_17,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x301, .note = "MDL1_GDMA",
	.name = "MDHWMIX_MDL1_GDMA_m_97" },
{ .master = MST_ID_MDHW_18,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x302, .note = "MDL1_ABM", .name = "MDHWMIX_MDL1_ABM_m_97" },
{ .master = MST_ID_MDHW_19,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x400, .note = "ARM7", .name = "MDHWMIX_ARM7_m_97" },
{ .master = MST_ID_MDHW_20,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x500, .note = "PS_PERI",
	.name = "MDHWMIX_PS_PERI_m_97" },
{ .master = MST_ID_MDHW_21,  .port = 0x4, .id_mask = 0x1FFE,
	.id_val = 0x600, .note = "LTEL2DMA", .name = "MDHWMIX_LTEL2DMA_m_97" },
{ .master = MST_ID_MDHW_22,  .port = 0x4, .id_mask = 0x1FFE,
	.id_val = 0x700, .note = "SOE_TRACE",
	.name = "MDHWMIX_SOE_TRACE_m_97" },
{ .master = MST_ID_MDHW_23,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x800, .note = "CMP_DSPLOG",
	.name = "MDHWMIX_CMP_DSPLOG_m_97" },
{ .master = MST_ID_MDHW_24,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x800, .note = "CMP_DSPBTDMA",
	.name = "MDHWMIX_CMP_DSPBTDMA_m_97" },
{ .master = MST_ID_MDHW_25,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x802, .note = "CMP_CNWDMA",
	.name = "MDHWMIX_CMP_CNWDMA_m_97" },
{ .master = MST_ID_MDHW_26,  .port = 0x4, .id_mask = 0x1FFD,
	.id_val = 0x810, .note = "CS", .name = "MDHWMIX_CS_m_97" },
{ .master = MST_ID_MDHW_27,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x820, .note = "IMC_RXTDB",
	.name = "MDHWMIX_IMC_RXTDB_m_97" },
{ .master = MST_ID_MDHW_28,  .port = 0x4, .id_mask = 0x1FF7,
	.id_val = 0x822, .note = "IMC_DSPLOG",
	.name = "MDHWMIX_IMC_DSPLOG_m_97" },
{ .master = MST_ID_MDHW_29,  .port = 0x4, .id_mask = 0x1FF7,
	.id_val = 0x822, .note = "IMC_DSPBTDMA",
	.name = "MDHWMIX_IMC_DSPBTDMA_m_97" },
{ .master = MST_ID_MDHW_30,  .port = 0x4, .id_mask = 0x1FF7,
	.id_val = 0x826, .note = "IMC_CNWDMA",
	.name = "MDHWMIX_IMC_CNWDMA_m_97" },
{ .master = MST_ID_MDHW_31,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x830, .note = "ICC_DSPLOG",
	.name = "MDHWMIX_ICC_DSPLOG_m_97" },
{ .master = MST_ID_MDHW_32,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x830, .note = "ICC_DSPBTDMA",
	.name = "MDHWMIX_ICC_DSPBTDMA_m_97" },
{ .master = MST_ID_MDHW_33,  .port = 0x4, .id_mask = 0x1FFB,
	.id_val = 0x832, .note = "ICC_CNWDMA",
	.name = "MDHWMIX_ICC_CNWDMA_m_97" },
{ .master = MST_ID_MDHW_34,  .port = 0x4, .id_mask = 0x1FFD,
	.id_val = 0x840, .note = "RXDMP", .name = "MDHWMIX_RXDMP_m_97" },
{ .master = MST_ID_MDHW_35,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x801, .note = "cs_id0",	.name = "MDHWMIX_cs_id0_m_97" },
{ .master = MST_ID_MDHW_36,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x803, .note = "cs_id1", .name = "MDHWMIX_cs_id1_m_97" },
{ .master = MST_ID_MDHW_37,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x1000, .note = "CLDMA_AP", .name = "MDHWMIX_CLDMA_AP_m_97" },
{ .master = MST_ID_MDHW_38,  .port = 0x4, .id_mask = 0x1FFF,
	.id_val = 0x1001, .note = "CLDMA_MD", .name = "MDHWMIX_CLDMA_MD_m_97" },

/* MM ch2 */
{ .master = MST_ID_MM_CH2_0, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x0, .note = "smi_larb0", .name = "smi_larb0_m_97" },
{ .master = MST_ID_MM_CH2_1, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x80, .note = "smi_larb1", .name = "smi_larb1_m_97" },
{ .master = MST_ID_MM_CH2_2, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x100, .note = "smi_larb2", .name = "smi_larb2_m_97" },
{ .master = MST_ID_MM_CH2_3, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x180, .note = "smi_larb3", .name = "smi_larb3_m_97" },
{ .master = MST_ID_MM_CH2_4, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x200, .note = "smi_larb4", .name = "smi_larb4_m_97" },
{ .master = MST_ID_MM_CH2_5, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x280, .note = "smi_larb5", .name = "smi_larb5_m_97" },
{ .master = MST_ID_MM_CH2_6, .port = 0x5, .id_mask = 0x1F80,
	.id_val = 0x300, .note = "smi_larb6", .name = "smi_larb6_m_97" },
{ .master = MST_ID_MM_CH2_7, .port = 0x5, .id_mask = 0x1FFF,
	.id_val = 0x3FC, .note = "MM IOMMU Internal Used",
	.name = "MM_IOMMU_Internal_Used_m_97" },
{ .master = MST_ID_MM_CH2_8, .port = 0x5, .id_mask = 0x1FFF,
	.id_val = 0x3FD, .note = "MM IOMMU Internal Used",
	.name = "MM_IOMMU_Internal_Used_m_97" },

/* Periperal */
{ .master = MST_ID_PERI_0,  .port = 0x6, .id_mask = 0x1FF7,
	.id_val = 0x6, .note = "DebugTop", .name = "DebugTop_m_97" },
{ .master = MST_ID_PERI_1,  .port = 0x6, .id_mask = 0x1FF7,
	.id_val = 0x0, .note = "USB3", .name = "USB_m_97" },
{ .master = MST_ID_PERI_2,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x1, .note = "USB2", .name = "USB_m_97" },
{ .master = MST_ID_PERI_3,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x21, .note = "Audio", .name = "Audio_m_97" },
{ .master = MST_ID_PERI_4,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x5, .note = "PWM", .name = "PWM_m_97" },
{ .master = MST_ID_PERI_5,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x25, .note = "MSDC1", .name = "MSDC_m_97" },
{ .master = MST_ID_PERI_6,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x45, .note = "MSDC2", .name = "MSDC2_m_97" },
{ .master = MST_ID_PERI_7,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x65, .note = "SPI0", .name = "SPI_m_97" },
{ .master = MST_ID_PERI_8,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x9, .note = "CONNSYS_SPI1_MD1_C2K",
	.name = "CONNSYS_SPI1_MD1_C2K_m_97" },
{ .master = MST_ID_PERI_9,  .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x29, .note = "SPM", .name = "SPM_m_97" },
{ .master = MST_ID_PERI_10, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x49, .note = "SCP", .name = "SCP_m_97" },
{ .master = MST_ID_PERI_11, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x69, .note = "THERM", .name = "THERM_m_97" },
{ .master = MST_ID_PERI_12, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x15, .note = "SPI2", .name = "SPI_m_97" },
{ .master = MST_ID_PERI_13, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x35, .note = "SPI3", .name = "SPI_m_97" },
{ .master = MST_ID_PERI_14, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x55, .note = "SPI4", .name = "SPI_m_97" },
{ .master = MST_ID_PERI_15, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x75, .note = "SPI5", .name = "SPI_m_97" },
{ .master = MST_ID_PERI_16, .port = 0x6, .id_mask = 0x1FDF,
	.id_val = 0x11, .note = "DMA", .name = "DMA_m_97" },
{ .master = MST_ID_PERI_17, .port = 0x6, .id_mask = 0x1FDF,
	.id_val = 0xD, .note = "MSDC0", .name = "MSDC_m_97" },
{ .master = MST_ID_PERI_18, .port = 0x6, .id_mask = 0x1FFB,
	.id_val = 0x3, .note = "CONNSYS", .name = "CONNSYS_m_97" },
{ .master = MST_ID_PERI_19, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x2, .note = "GCPU_M", .name = "GCPU_M_m_97" },
{ .master = MST_ID_PERI_20, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x4, .note = "GCE", .name = "GCE_m_97" },
{ .master = MST_ID_PERI_21, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0xC, .note = "GDMA", .name = "GDMA_m_97" },
{ .master = MST_ID_PERI_22, .port = 0x6, .id_mask = 0x1FFF,
	.id_val = 0x14, .note = "GCE_prefetch", .name = "GCE_prefetch_m_97" },

	/* MFG */
	{ .master = MST_ID_MFG_0, .port = 0x7, .id_mask = 0x1FC0,
		.id_val = 0x0, .note = "MFG", .name = "MFG_m_97" },

};

static const char *UNKNOWN_MASTER = "unknown";
static spinlock_t emi_mpu_lock;

#ifdef ENABLE_EMI_CHKER
struct timer_list emi_axi_vio_timer;
#endif

char *smi_larb0_port[7] = {
	"LARB0_display_m_97", "LARB0_display_m_97", "LARB0_display_m_97",
	"LARB0_display_m_97", "LARB0_MDP_m_97", "LARB0_MDP_m_97",
	"LARB0_MDP_m_97"};
char *smi_larb1_port[10] =  {
	"LARB1_VDEC_m_97", "LARB1_VDEC_m_97",
	"LARB1_VDEC_m_97", "LARB1_VDEC_m_97",
	"LARB1_VDEC_m_97", "LARB1_VDEC_m_97",
	"LARB1_VDEC_m_97", "LARB1_VDEC_m_97", "LARB1_VDEC_m_97",
	"LARB1_VDEC_m_97"};
char *smi_larb2_port[14] = {
	"LARB2_ISP_m_97", "LARB2_ISP_m_97", "LARB2_ISP_m_97",
	"LARB2_ISP_m_97", "LARB2_ISP_m_97", "LARB2_ISP_m_97",
	"LARB2_ISP_m_97", "LARB2_ISP_m_97", "LARB2_ISP_m_97",
	"LARB2_ISP_m_97", "LARB2_ISP_m_97", "LARB2_ISP_m_97",
	"LARB2_ISP_m_97", "LARB2_ISP_m_97"};
char *smi_larb3_port[15] = {
	"LARB3_VENC_m_97", "LARB3_VENC_m_97", "LARB3_VENC_m_97",
	"LARB3_VENC_m_97", "LARB3_VENC_m_97", "LARB3_JPEG_m_97",
	"LARB3_JPEG_m_97", "LARB3_JPEG_m_97", "LARB3_JPEG_m_97",
	"LARB3_VENC_m_97", "LARB3_VENC_m_97", "LARB3_VENC_m_97",
	"LARB3_VENC_m_97", "LARB3_VENC_m_97", "LARB3_VENC_m_97"};
char *smi_larb4_port[5] = {
	"LARB4_MJC_m_97", "LARB4_MJC_m_97", "LARB4_MJC_m_97",
	"LARB4_MJC_m_97", "LARB4_MJC_m_97"};
char *smi_larb5_port[9] = {
	"LARB5_display_m_97", "LARB5_display_m_97", "LARB5_display_m_97",
	"LARB5_display_m_97", "LARB5_OD_m_97", "LARB5_OD_m_97",
	"LARB5_display_m_97", "LARB5_MDP_m_97", "LARB5_MDP_m_97"};
char *smi_larb6_port[10] = {
	"LARB6_ISP_m_97", "LARB6_ISP_m_97", "LARB6_ISP_m_97",
	"LARB6_ISP_m_97", "LARB6_ISP_m_97", "LARB6_Face_m_97",
	"LARB6_Face_m_97", "LARB6_Face_m_97", "LARB6_Depth_m_97",
	"LARB6_Depth_m_97"};

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
		case 4:	/* MASTER_MDHW */
		case 6:	/* MASTER_PERI */
		case 7:	/* MASTER_MFG */
			pr_err("Violation master name is %s.\n",
			mst_tbl[tbl_idx].name);
			break;
		case 2: /* MASTER_MM_CH1 */
		case 5:	/* MASTER_MM_CH2 */
			 /*MM*/ mm_larb = axi_id >> 7;
			smi_port = (axi_id & 0x7F) >> 2;
			if (mm_larb == 0x0) {	/* smi_larb0 */
				if (smi_port >= ARRAY_SIZE(smi_larb0_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb0_port[smi_port]);
			} else if (mm_larb == 0x1) {	/* smi_larb1 */
				if (smi_port >= ARRAY_SIZE(smi_larb1_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb1_port[smi_port]);
			} else if (mm_larb == 0x2) {	/* smi_larb2 */
				if (smi_port >= ARRAY_SIZE(smi_larb2_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb2_port[smi_port]);
			} else if (mm_larb == 0x3) {	/* smi_larb3 */
				if (smi_port >= ARRAY_SIZE(smi_larb3_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb3_port[smi_port]);
			} else if (mm_larb == 0x4) {	/* smi_larb4 */
				if (smi_port >= ARRAY_SIZE(smi_larb4_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb4_port[smi_port]);
			} else if (mm_larb == 0x5) {	/* smi_larb5 */
				if (smi_port >= ARRAY_SIZE(smi_larb5_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb5_port[smi_port]);
			} else if (mm_larb == 0x6) {	/* smi_larb6 */
				if (smi_port >= ARRAY_SIZE(smi_larb6_port)) {
					pr_err("[EMI MPU ERROR] Invalidate master ID! lookup smi table failed!\n");
					return 0;
				}
				pr_err("Violation master name is %s (%s).\n",
				mst_tbl[tbl_idx].name,
				smi_larb6_port[smi_port]);
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

#if 0
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
#endif

static int isetocunt;
static void emi_mpu_set_violation_port(int portID)
{
	if (isetocunt == 0) {
			Violation_PortID = portID;
			isetocunt = 1;
		}
	else if (isetocunt == 2) {
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
					return smi_larb2_port[smi_portID];
				else if (mm_larbID == 0x3)
					return smi_larb3_port[smi_portID];
				else if (mm_larbID == 0x4)
					return smi_larb4_port[smi_portID];
				else if (mm_larbID == 0x5)
					return smi_larb5_port[smi_portID];
				else if (mm_larbID == 0x6)
					return smi_larb6_port[smi_portID];
				else
					return mst_tbl[i].name;
			} else
				return mst_tbl[i].name;
		}
	}

	return (char *)UNKNOWN_MASTER;
}
#define NR_REGIONS  24
static void __clear_emi_mpu_vio(unsigned int first)
{
	u32 dbg_s, dbg_t;
	/* clear violation status */

	writel(0x0FFFF3FF, EMI_MPUP);
	writel(0x0FFFF3FF, EMI_MPUQ);
	writel(0x0FFFF3FF, EMI_MPUR);
	writel(0x0FFFF3FF, EMI_MPUY);
	writel(0x0FFFF3FF, EMI_MPUP2);
	writel(0x0FFFF3FF, EMI_MPUQ2);
	writel(0x0FFFF3FF, EMI_MPUR2);
	mt_reg_sync_writel(0x0FFFF3FF, EMI_MPUY2);

	/* clear debug info */
	mt_reg_sync_writel(0x80000000 , EMI_MPUS);
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
	u32 dbg_s, dbg_t, dbg_pqry;
	u32 master_ID, domain_ID, wr_vio;
	s32 region;
	char *master_name;

	dbg_s = readl(IOMEM(EMI_MPUS));
	dbg_t = readl(IOMEM(EMI_MPUT));

	pr_alert("Clear status.\n");

	master_ID = (dbg_s & 0x0000FFFF);
	domain_ID = (dbg_s >> 21) & 0x00000007;
	wr_vio = (dbg_s >> 28) & 0x00000003;
	region = (dbg_s >> 16) & 0x1F;

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

#ifdef CONFIG_MTK_AEE_FEATURE
	if (wr_vio != 0) {
		/* EMI violation is relative to MD at user build*/
		#if 0 /* #ifndef CONFIG_MT_ENG_BUILD */
			if (((master_ID & 0x7) == MASTER_MDMCU) ||
				((master_ID & 0x7) == MASTER_MDHW)) {
					int md_id = 0;

					exec_ccci_kern_func_by_md_id(md_id,
					ID_FORCE_MD_ASSERT, NULL, 0);
					pr_err("[EMI MPU] MPU violation trigger MD\n");
				}
		#else
			if (((master_ID & 0x7) == MASTER_MDMCU) ||
				((master_ID & 0x7) == MASTER_MDHW)) {
					char str[60] = "0";
					char *pstr = str;

					sprintf(pstr, "EMI_MPUS = 0x%x, ADDR = 0x%x",
						dbg_s, dbg_t + emi_physical_offset);

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
		if ((region == 0) && (mt_emi_reg_read(EMI_MPUA) == 0)
			&& (mt_emi_reg_read(EMI_MPUI) == 0)
			&& (!(dbg_pqry & OOR_VIO))) {
				pr_err("[EMI MPU] A strange violation.\n");
		} else {
		aee_kernel_exception("EMI MPU",
"%sEMI_MPUS = 0x%x,EMI_MPUT = 0x%x\n CHKER = 0x%x,CHKER_TYPE = 0x%x,CHKER_ADR = 0x%x\n%s%s\n",
				     "EMI MPU violation.\n",
				     dbg_s,
				     dbg_t+emi_physical_offset,
				     readl(IOMEM(EMI_CHKER)),
				     readl(IOMEM(EMI_CHKER_TYPE)),
				     readl(IOMEM(EMI_CHKER_ADR)),
				     "CRDISPATCH_KEY:EMI MPU Violation Issue/",
				     master_name);
		}
		}
#endif

	__clear_emi_mpu_vio(0);
	mt_devapc_clear_emi_violation();
	vio_addr = dbg_t + emi_physical_offset;
	return 0;

}

/*EMI MPU violation handler*/
static irqreturn_t mpu_violation_irq(int irq, void *dev_id)
{
	int res, res1;

	/* Need DEVAPC owner porting */
	res = mt_devapc_check_emi_violation();
	if (!res)
		pr_err("it's a violation.\n");

	res1 = mt_devapc_check_emi_mpu_violation();
	if (!res1) {
		pr_err("EMI MPU APB violation.\n");
			pr_err("[EMI MPU] Debug info start ----------------------------------------\n");
			pr_err("EMI_MPUW=%x\n", mt_emi_reg_read(EMI_MPUW));
			pr_err("EMI_MPUV=%x\n", mt_emi_reg_read(EMI_MPUV));
			pr_err("EMI_MPUX=%x\n", mt_emi_reg_read(EMI_MPUX));
			pr_err("Violation address=%x\n",
			mt_emi_reg_read(EMI_MPUW) & 0xFFFF);
			pr_err("Violation domain=%x\n",
			(mt_emi_reg_read(EMI_MPUW)>>16) & 0x7);

			if ((mt_emi_reg_read(EMI_MPUV)>>28) & 0x1)
				pr_err("it's a Write violation\n");
			else if ((mt_emi_reg_read(EMI_MPUV)>>29) & 0x1)
				pr_err("it's a Read violation\n");

			pr_err("[EMI MPU] Debug info end------------------------------------------\n");
			mt_emi_reg_write(mt_emi_reg_read(EMI_MPUX)|0x80000000,
			EMI_MPUX);
			mt_devapc_clear_emi_mpu_violation();
			return IRQ_HANDLED;
		}

	if (res && res1)
		return IRQ_NONE;

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

	pasrdpd->channel_nr = ch_nr;

	emi_cona = readl(IOMEM(EMI_CONA));
	emi_conh = readl(IOMEM(EMI_CONH));

	ch0_rank0_size = (emi_conh >> 16) & 0xf;
	ch0_rank1_size = (emi_conh >> 20) & 0xf;
	ch1_rank0_size = (emi_conh >> 24) & 0xf;
	ch1_rank1_size = (emi_conh >> 28) & 0xf;

	{
		pasrdpd->channel[0].rank[0].valid_rank = true;

		if (ch0_rank0_size == 0) {
			col_bit = ((emi_cona >> 4) & 0x03) + 9;
			row_bit = ((emi_cona >> 12) & 0x03) + 13;
			pasrdpd->channel[0].rank[0].rank_size =
			(1 << (row_bit + col_bit)) >> 22;
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
				row_bit = ((emi_cona >> 14) & 0x03) + 13;
				pasrdpd->channel[0].rank[1].rank_size =
				(1 << (row_bit + col_bit)) >> 22;
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

	if (0 != (emi_cona & 0x01)) {

		pasrdpd->channel[1].rank[0].valid_rank = true;

		if (ch1_rank0_size == 0) {
			col_bit = ((emi_cona >> 20) & 0x03) + 9;
			row_bit = ((emi_cona >> 28) & 0x03) + 13;
			pasrdpd->channel[1].rank[0].rank_size =
			(1 << (row_bit + col_bit)) >> 22;
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
				row_bit = ((emi_cona >> 30) & 0x03) + 13;
				pasrdpd->channel[1].rank[1].rank_size =
				(1 << (row_bit + col_bit)) >> 22;
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
	unsigned int start, end;
	unsigned int reg_value;
	unsigned int d0, d1, d2, d3, d4, d5, d6, d7;
	static const char *permission[7] = {
		"No",
		"S_RW",
		"S_RW_NS_R",
		"S_RW_NS_W",
		"S_R_NS_R",
		"FORBIDDEN",
		"S_R_NS_RW"
	};

	reg_value = mt_emi_reg_read(EMI_MPUA);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R0-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUB);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R1-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUC);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R2-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUD);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R3-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUE);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R4-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUF);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R5-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUG);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R6-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUH);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R7-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUA2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R8-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUB2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R9-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUC2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R10-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUD2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R11-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUE2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R12-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUF2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R13-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUG2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R14-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUH2);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R15-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUA3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R16-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUB3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R17-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUC3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R18-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUD3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R19-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUE3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R20-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUF3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R21-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUG3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R22-> 0x%x to 0x%x\n", start, end);

	reg_value = mt_emi_reg_read(EMI_MPUH3);
	start = ((reg_value >> 16) << 16) + emi_physical_offset;
	end = ((reg_value & 0xFFFF) << 16) + emi_physical_offset + 0xFFFF;
	ptr += sprintf(ptr, "R23-> 0x%x to 0x%x\n", start, end);

	ptr += sprintf(ptr, "\n");

	reg_value = mt_emi_reg_read(EMI_MPUI);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R0-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R0-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUJ);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R1-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R1-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUK);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R2-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R2-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUL);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R3-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R3-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);


	reg_value = mt_emi_reg_read(EMI_MPUI_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R4-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R4-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);


	reg_value = mt_emi_reg_read(EMI_MPUJ_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R5-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R5-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUK_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R6-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R6-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUL_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R7-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R7-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUI2);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R8-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R8-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUJ2);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R9-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R9-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUK2);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R10-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R10-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUL2);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R11-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R11-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUI2_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R12-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R12-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUJ2_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R13-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R13-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUK2_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R14-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R14-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUL2_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R15-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R15-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUI3);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R16-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R16-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUJ3);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R17-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R17-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUK3);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R18-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R18-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUL3);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R19-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R19-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUI3_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R20-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R20-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUJ3_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R21-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R21-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUK3_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R22-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R22-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);

	reg_value = mt_emi_reg_read(EMI_MPUL3_2ND);
	d0 = (reg_value & 0x7);
	d1 = (reg_value >> 3) & 0x7;
	d2 = (reg_value >> 6) & 0x7;
	d3 = (reg_value >> 9) & 0x7;
	d4 = (reg_value >> 12) & 0x7;
	d5 = (reg_value >> 15) & 0x7;
	d6 = (reg_value >> 18) & 0x7;
	d7 = (reg_value >> 21) & 0x7;
	ptr += sprintf(ptr, "R23-> d0 = %s, d1 = %s, d2 = %s, d3 = %s\n",
	permission[d0],  permission[d1],  permission[d2], permission[d3]);
	ptr += sprintf(ptr, "R23-> d4 = %s, d5 = %s, d6 = %s, d7 = %s\n",
	permission[d4],  permission[d5],  permission[d6], permission[d7]);
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
		start_addr = simple_strtoul(token[1], &token[1], 16);
		end_addr = simple_strtoul(token[2], &token[2], 16);
		region = simple_strtoul(token[3], &token[3], 16);
		access_permission = simple_strtoul(token[4], &token[4], 16);
		*/
		ret = kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse start_addr");
		ret = kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse end_addr");
		ret = kstrtoul(token[3], 16, (unsigned long *)&region);
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
		for (i = 0; i < 5; i++)
			pr_devel("token[%d] = %s\n", i, token[i]);
		/* kernel standardization
		start_addr = simple_strtoul(token[1], &token[1], 16);
		end_addr = simple_strtoul(token[2], &token[2], 16);
		region = simple_strtoul(token[3], &token[3], 16);
		*/
		ret = kstrtoul(token[1], 16, (unsigned long *)&start_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse start_addr");
		ret = kstrtoul(token[2], 16, (unsigned long *)&end_addr);
		if (ret != 0)
			pr_err("kstrtoul fails to parse end_addr");
		ret = kstrtoul(token[3], 16, (unsigned long *)&region);
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
	unsigned int v_addr = vio_addr;

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
					pr_err("[EMI MPU] Find page entry section at pte: %lx. violation address = 0x%x\n",
					(unsigned long)(pte), v_addr);
					return;
				}
			}
		} else {
			/* Section */
			if (((unsigned long)pmd_val(*(pmd)) & SECTION_MASK) ==
				((unsigned long)v_addr
				& SECTION_MASK)) {
				pr_err("[EMI MPU] Find page entry section at pmd: %lx. violation address = 0x%x\n",
				(unsigned long)(pmd), v_addr);
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

		pr_err("[EMI MPU AXI] Debug info end ----------------------------------------\n");

		master_name = __id2name(master_ID);
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
	and timer will be unavailable */

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
	unsigned int value, master_ID, type, vio_addr;
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
	unsigned int mpu_irq;

	pr_info("Initialize EMI MPU.\n");

	isetocunt = 0;

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

	if (INFRA_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL,
		"mediatek,infracfg_ao");
		if (node) {
			INFRA_BASE_ADDR = of_iomap(node, 0);
			pr_err("get INFRA_BASE_ADDR @ %p\n", INFRA_BASE_ADDR);
		} else {
			pr_err("can't find compatible node INFRA_BASE_ADDR\n");
			return -1;
		}
	}

	if (INFRACFG_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg");
		if (node) {
			INFRACFG_BASE_ADDR = of_iomap(node, 0);
			pr_err("get INFRACFG_BASE_ADDR @ %p\n",
			INFRACFG_BASE_ADDR);
		} else {
			pr_err("can't find compatible node INFRACFG_BASE_ADDR\n");
			return -1;
		}
	}

	if (SMI_BASE_ADDR == NULL) {
		node = of_find_compatible_node(NULL, NULL, "mediatek,mdhw_smi");
		if (node) {
			SMI_BASE_ADDR = of_iomap(node, 0);
			pr_err("get SMI_BASE_ADDR @ %p\n", SMI_BASE_ADDR);
		} else {
			pr_err("can't find compatible node SMI_BASE_ADDR\n");
			return -1;
		}
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,devapc");
	if (node) {
		mpu_irq = irq_of_parse_and_map(node, 0);
		pr_notice("get EMI_MPU irq = %d\n", mpu_irq);
	} else {
		pr_err("can't find compatible node\n");
		return -1;
	}

	spin_lock_init(&emi_mpu_lock);

	pr_err("[EMI MPU] EMI_MPUP = 0x%x\n", readl(IOMEM(EMI_MPUP)));
	pr_err("[EMI MPU] EMI_MPUQ = 0x%x\n", readl(IOMEM(EMI_MPUQ)));
	pr_err("[EMI MPU] EMI_MPUR = 0x%x\n", readl(IOMEM(EMI_MPUR)));
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

	pr_err("[EMI MPU] EMI_MPUW = 0x%x\n", mt_emi_reg_read(EMI_MPUW));
	pr_err("[EMI MPU] EMI_MPUV = 0x%x\n", mt_emi_reg_read(EMI_MPUV));
	pr_err("[EMI MPU] EMI_MPUX = 0x%x\n", mt_emi_reg_read(EMI_MPUX));

	if (readl(IOMEM(EMI_MPUS))) {
		pr_err("[EMI MPU] get MPU violation in driver init\n");
		mt_devapc_emi_initial();
		mpu_check_violation();
	} else {
		__clear_emi_mpu_vio(0);
		/* Set Device APC initialization for EMI-MPU. */
		mt_devapc_emi_initial();
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

	ret = request_irq(mpu_irq,
	 (irq_handler_t)mpu_violation_irq, IRQF_TRIGGER_LOW | IRQF_SHARED,
	 "mt_emi_mpu", &emi_mpu_ctrl);
	if (ret != 0) {
		pr_err("Fail to request EMI_MPU interrupt. Error = %d.\n", ret);
		return ret;
	}

	protect_ap_region();

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
		pr_err("[EMI MPU] Not 4G mode\n");
	} else { /* enable 4G mode */
		enable_4gb = 1;
		pr_err("[EMI MPU] 4G mode\n");
	}

	return 0;
}

early_initcall(dram_4gb_init);
#endif

#define INFRA_TOP_DBG_CON0                     IOMEM(INFRACFG_BASE_ADDR+0x100)
#define INFRA_TOP_DBG_CON1                     IOMEM(INFRACFG_BASE_ADDR+0x104)
#define INFRA_TOP_DBG_CON2                     IOMEM(INFRACFG_BASE_ADDR+0x108)
#define INFRA_TOP_DBG_CON3                     IOMEM(INFRACFG_BASE_ADDR+0x10C)

#define EMI_DFTB                     IOMEM(EMI_BASE_ADDR+0x0E8)
#define EMI_APBDBG                     IOMEM(EMI_BASE_ADDR+0x5FC)

#define MEM_DCM_CTRL                     IOMEM(INFRA_BASE_ADDR+0x78)

#define EMI_CONM                       IOMEM(EMI_BASE_ADDR+0x060)
#define EMI_BMEN                        IOMEM(EMI_BASE_ADDR+0x400)
#define EMI_MSEL                         IOMEM(EMI_BASE_ADDR+0x440)
#define EMI_MSEL2                       IOMEM(EMI_BASE_ADDR+0x468)
#define EMI_MSEL3                       IOMEM(EMI_BASE_ADDR+0x470)
#define EMI_MSEL4                       IOMEM(EMI_BASE_ADDR+0x478)
#define EMI_MSEL5                       IOMEM(EMI_BASE_ADDR+0x480)
#define EMI_MSEL6                       IOMEM(EMI_BASE_ADDR+0x488)
#define EMI_MSEL7                       IOMEM(EMI_BASE_ADDR+0x490)
#define EMI_MSEL8                       IOMEM(EMI_BASE_ADDR+0x498)
#define EMI_BMEN2                     IOMEM(EMI_BASE_ADDR+0x4E8)
#define EMI_BMRW0                    IOMEM(EMI_BASE_ADDR+0x4F8)
#define EMI_WSCT                         IOMEM(EMI_BASE_ADDR+0x428)
#define EMI_WSCT2                      IOMEM(EMI_BASE_ADDR+0x458)
#define EMI_WSCT3                      IOMEM(EMI_BASE_ADDR+0x460)
#define EMI_WSCT4                      IOMEM(EMI_BASE_ADDR+0x464)
#define EMI_TTYPE1                      IOMEM(EMI_BASE_ADDR+0x500)
#define EMI_TTYPE9                      IOMEM(EMI_BASE_ADDR+0x540)
#define EMI_TTYPE2                      IOMEM(EMI_BASE_ADDR+0x508)
#define EMI_TTYPE10                      IOMEM(EMI_BASE_ADDR+0x548)
#define EMI_TTYPE3                      IOMEM(EMI_BASE_ADDR+0x510)
#define EMI_TTYPE11                      IOMEM(EMI_BASE_ADDR+0x550)
#define EMI_TTYPE4                      IOMEM(EMI_BASE_ADDR+0x518)
#define EMI_TTYPE12                    IOMEM(EMI_BASE_ADDR+0x558)
#define EMI_TTYPE5                      IOMEM(EMI_BASE_ADDR+0x520)
#define EMI_TTYPE13                    IOMEM(EMI_BASE_ADDR+0x560)
#define EMI_TTYPE6                      IOMEM(EMI_BASE_ADDR+0x528)
#define EMI_TTYPE14                    IOMEM(EMI_BASE_ADDR+0x568)
#define EMI_TTYPE7                      IOMEM(EMI_BASE_ADDR+0x530)
#define EMI_TTYPE15                    IOMEM(EMI_BASE_ADDR+0x570)
#define EMI_TTYPE8                    IOMEM(EMI_BASE_ADDR+0x538)
#define EMI_TTYPE16                    IOMEM(EMI_BASE_ADDR+0x578)
#define EMI_CONI                          IOMEM(EMI_BASE_ADDR+0x040)
#define EMI_TEST0                         IOMEM(EMI_BASE_ADDR+0x0D0)
#define EMI_TEST1                         IOMEM(EMI_BASE_ADDR+0x0D8)
void dump_emi_latency(void)
{
mt_reg_sync_writel(0x00000060, INFRA_TOP_DBG_CON0);
mt_reg_sync_writel(0x00000080, INFRA_TOP_DBG_CON1);
mt_reg_sync_writel(0x00000360, INFRA_TOP_DBG_CON2);
mt_reg_sync_writel(0x000004E0, INFRA_TOP_DBG_CON3);

mt_reg_sync_writel(readl(EMI_DFTB) | (1<<8),       EMI_DFTB);

/* Read EMI debug out */
pr_err("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));
/* Disable EMI DCM */
pr_err("Disable EMI DCM\n");
mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~(1<<7)), MEM_DCM_CTRL);
mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (0x1f<<1), MEM_DCM_CTRL);
mt_reg_sync_writel(readl(MEM_DCM_CTRL) | 0x1,       MEM_DCM_CTRL);
mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~0x1),    MEM_DCM_CTRL);

/* Enable EMI bus monitor */
pr_err("Enable EMI bus monitor\n");
mt_reg_sync_writel(readl(EMI_CONM) | (1<<30), EMI_CONM);
mt_reg_sync_writel(0x00FF0000, EMI_BMEN);
mt_reg_sync_writel(0x00100003, EMI_MSEL);
mt_reg_sync_writel(0x00000008, EMI_MSEL2);
mt_reg_sync_writel(0x02000000, EMI_BMEN2);
mt_reg_sync_writel(0xAAAAAAAA, EMI_BMRW0);
mt_reg_sync_writel(readl(EMI_BMEN) | 0x1, EMI_BMEN);

mdelay(5);

pr_err("Get latency Result\n");
mt_reg_sync_writel(readl(EMI_BMEN) | 0x2, EMI_BMEN);
/* Read EMI debug out */
pr_err("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));

pr_err("-----------Get BW RESULT----------------\n");
/* Get BW result */
pr_err("EMI_WSCT = 0x%x\n", readl(EMI_WSCT));
pr_err("EMI_WSCT2 = 0x%x\n", readl(EMI_WSCT2));
pr_err("EMI_WSCT3 = 0x%x\n", readl(EMI_WSCT3));
pr_err("EMI_WSCT4 = 0x%x\n", readl(EMI_WSCT4));

/* Get latency result */
pr_err("EMI_TTYPE1 = 0x%x\n", readl(EMI_TTYPE1));
pr_err("EMI_TTYPE9 = 0x%x\n", readl(EMI_TTYPE9));
pr_err("EMI_TTYPE2 = 0x%x\n", readl(EMI_TTYPE2));
pr_err("EMI_TTYPE10 = 0x%x\n", readl(EMI_TTYPE10));
pr_err("EMI_TTYPE3 = 0x%x\n", readl(EMI_TTYPE3));
pr_err("EMI_TTYPE11 = 0x%x\n", readl(EMI_TTYPE11));
pr_err("EMI_TTYPE4 = 0x%x\n", readl(EMI_TTYPE4));
pr_err("EMI_TTYPE12 = 0x%x\n", readl(EMI_TTYPE12));
pr_err("EMI_TTYPE5 = 0x%x\n", readl(EMI_TTYPE5));
pr_err("EMI_TTYPE13 = 0x%x\n", readl(EMI_TTYPE13));
pr_err("EMI_TTYPE6 = 0x%x\n", readl(EMI_TTYPE6));
pr_err("EMI_TTYPE14 = 0x%x\n", readl(EMI_TTYPE14));
pr_err("EMI_TTYPE7 = 0x%x\n", readl(EMI_TTYPE7));
pr_err("EMI_TTYPE15 = 0x%x\n", readl(EMI_TTYPE15));
pr_err("EMI_TTYPE8 = 0x%x\n", readl(EMI_TTYPE8));
pr_err("EMI_TTYPE16 = 0x%x\n", readl(EMI_TTYPE16));

pr_err("EMI_CONI  = 0x%x\n", readl(EMI_CONI));
pr_err("EMI_TEST0 = 0x%x\n", readl(EMI_TEST0));
pr_err("EMI_TEST1 = 0x%x\n", readl(EMI_TEST1));

/*read SMI debug register     // 0x1021C400 ~ 0x1021C434 */
pr_err("SMI debug register 0x1021C400= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x400)));
pr_err("SMI debug register 0x1021C404= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x404)));
pr_err("SMI debug register 0x1021C408= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x408)));
pr_err("SMI debug register 0x1021C40C= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x40C)));
pr_err("SMI debug register 0x1021C410= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x410)));
pr_err("SMI debug register 0x1021C414= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x414)));
pr_err("SMI debug register 0x1021C418= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x418)));
pr_err("SMI debug register 0x1021C41C= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x41C)));
pr_err("SMI debug register 0x1021C420= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x420)));
pr_err("SMI debug register 0x1021C424= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x424)));
pr_err("SMI debug register 0x1021C428= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x428)));
pr_err("SMI debug register 0x1021C42C= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x42C)));
pr_err("SMI debug register 0x1021C430= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x430)));
pr_err("SMI debug register 0x1021C434= 0x%x\n",
readl(IOMEM(SMI_BASE_ADDR+0x434)));
/*read Bus debug register     // 0x10201190 */
pr_err("Bus debug register 0x10201190= 0x%x\n",
readl(IOMEM(INFRACFG_BASE_ADDR+0x190)));

/* Read EMI debug out */
pr_err("EMI debug out EMI_APBDBG= 0x%x\n", readl(EMI_APBDBG));
}

void dump_emi_MM(void)
{
/*temp for MM, should be removed later */
/* Disable EMI DCM */
pr_err("Disable EMI DCM @ pgt_scan_show start\n");
mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~(1<<7)), MEM_DCM_CTRL);
mt_reg_sync_writel(readl(MEM_DCM_CTRL) | (0x1f<<1), MEM_DCM_CTRL);
mt_reg_sync_writel(readl(MEM_DCM_CTRL) | 0x1,       MEM_DCM_CTRL);
mt_reg_sync_writel(readl(MEM_DCM_CTRL) & (~0x1),    MEM_DCM_CTRL);

/* Enable EMI bus monitor */
pr_err("Enable EMI bus monitor\n");
mt_reg_sync_writel(readl(EMI_CONM) | (1<<30), EMI_CONM);
mt_reg_sync_writel(0xC8FF0000, EMI_BMEN);
mt_reg_sync_writel(0xCAFFC9FF, EMI_MSEL);
mt_reg_sync_writel(0xCCFFCBFF, EMI_MSEL2);
mt_reg_sync_writel(0xCEFFCDFF, EMI_MSEL3);
mt_reg_sync_writel(0x88FFCFFF, EMI_MSEL4);
mt_reg_sync_writel(0x8AFF89FF, EMI_MSEL5);
mt_reg_sync_writel(0x8CFF8BFF, EMI_MSEL6);
mt_reg_sync_writel(0x8EFF8DFF, EMI_MSEL7);
mt_reg_sync_writel(0x00008FFF, EMI_MSEL8);
mt_reg_sync_writel(readl(EMI_BMEN) | 0x1, EMI_BMEN);
pr_err("Disable EMI DCM @ pgt_scan_show end\n");

pr_err("Get latency Result\n");
mt_reg_sync_writel(readl(EMI_BMEN) | 0x2, EMI_BMEN);

pr_err("-----------Get BW RESULT----------------\n");

/* Get latency result */
pr_err("EMI_TTYPE1 = 0x%x\n", readl(EMI_TTYPE1));
pr_err("EMI_TTYPE9 = 0x%x\n", readl(EMI_TTYPE9));
pr_err("EMI_TTYPE2 = 0x%x\n", readl(EMI_TTYPE2));
pr_err("EMI_TTYPE10 = 0x%x\n", readl(EMI_TTYPE10));
pr_err("EMI_TTYPE3 = 0x%x\n", readl(EMI_TTYPE3));
pr_err("EMI_TTYPE11 = 0x%x\n", readl(EMI_TTYPE11));
pr_err("EMI_TTYPE4 = 0x%x\n", readl(EMI_TTYPE4));
pr_err("EMI_TTYPE12 = 0x%x\n", readl(EMI_TTYPE12));
pr_err("EMI_TTYPE5 = 0x%x\n", readl(EMI_TTYPE5));
pr_err("EMI_TTYPE13 = 0x%x\n", readl(EMI_TTYPE13));
pr_err("EMI_TTYPE6 = 0x%x\n", readl(EMI_TTYPE6));
pr_err("EMI_TTYPE14 = 0x%x\n", readl(EMI_TTYPE14));
pr_err("EMI_TTYPE7 = 0x%x\n", readl(EMI_TTYPE7));
pr_err("EMI_TTYPE15 = 0x%x\n", readl(EMI_TTYPE15));
pr_err("EMI_TTYPE8 = 0x%x\n", readl(EMI_TTYPE8));
pr_err("EMI_TTYPE16 = 0x%x\n", readl(EMI_TTYPE16));

}
