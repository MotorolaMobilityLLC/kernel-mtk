/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_IOMMU_H_
#define _MTK_IOMMU_H_

#include <linux/iommu.h>
#include <soc/mediatek/smi.h>

#include "io-pgtable.h"

struct mtk_iommu_suspend_reg {
	u32				standard_axi_mode;
	u32				dcm_dis;
	u32				ctrl_reg;
	u32				int_control0;
	u32				int_main_control;
};

struct mtk_iommu_client_priv {
	struct list_head		client;
	unsigned int			mtk_m4u_id;
	struct device			*m4udev;
};

struct mtk_iommu_domain {
	spinlock_t			pgtlock; /* lock for page table */

	struct io_pgtable_cfg		cfg;
	struct io_pgtable_ops		*iop;

	struct iommu_domain		domain;
};

struct mtk_iommu_data {
	void __iomem			*base;
	int				irq;
	struct device			*dev;
	struct clk			*bclk;
	phys_addr_t			protect_base; /* protect memory base */
	struct mtk_iommu_suspend_reg	reg;
	struct mtk_iommu_domain		*m4u_dom;
	struct iommu_group		*m4u_group;
	struct mtk_smi_iommu		smi_imu;      /* SMI larb iommu info */
	bool                            enable_4GB;
};

#ifdef CONFIG_MTK_IOMMU
unsigned long mtk_get_pgt_base(void);
extern phys_addr_t mtkfb_get_fb_base(void);
#ifndef CONFIG_MTK_SMI_EXT
size_t mtkfb_get_fb_size(void);
#endif
int smi_reg_backup_sec(void);
int smi_reg_restore_sec(void);

typedef enum mtk_iommu_callback_ret {
	MTK_IOMMU_CALLBACK_HANDLED,
	MTK_IOMMU_CALLBACK_NOT_HANDLED,
} mtk_iommu_callback_ret_t;

typedef mtk_iommu_callback_ret_t (mtk_iommu_fault_callback_t)(int port, unsigned int mva, void *cb_data);
int mtk_iommu_register_fault_callback(int port, mtk_iommu_fault_callback_t *fn, void *cb_data);
int mtk_iommu_unregister_fault_callback(int port);
int mtk_iommu_enable_tf(int port, bool fgenable);
void *mtk_iommu_iova_to_va(struct device *dev, dma_addr_t iova, size_t size);

#else
static unsigned long mtk_get_pgt_base(void)
{
	return 0;
}

#endif

#endif
