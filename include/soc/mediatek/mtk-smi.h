/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
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
#ifndef MTK_IOMMU_SMI_H
#define MTK_IOMMU_SMI_H
#include <linux/device.h>

#ifdef CONFIG_MTK_SMI
/*
 * Enable iommu for each port, it is only for iommu.
 *
 * Returns 0 if successfully, others if failed.
 */
int mtk_smi_config_port(struct device *larbdev, unsigned int larbportid,
			bool iommuen);
/*
 * The two function below help open/close the clock of the larb.
 *
 * mtk_smi_larb_get must be called before the multimedia h/w work.
 * mtk_smi_larb_put must be called after h/w done.
 * Both should be called in non-atomic context.
 *
 * Returns 0 if successfully, others if failed.
 */
int mtk_smi_larb_get(struct device *plarbdev);
void mtk_smi_larb_put(struct device *plarbdev);

#else

static inline int
mtk_smi_config_port(struct device *larbdev, unsigned int larbportid,
		    bool iommuen)
{
	return 0;
}
static inline int mtk_smi_larb_get(struct device *plarbdev)
{
	return 0;
}
static inline void mtk_smi_larb_put(struct device *plarbdev) { }

#endif

#endif
