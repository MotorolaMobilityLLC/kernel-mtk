/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Maoguang.Meng <maoguang.meng@mediatek.com>
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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/types.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip/mtk-gic-extend.h>


void __iomem *GIC_DIST_BASE;
void __iomem *INT_POL_CTL0;

/*
 * mt_irq_mask_all: disable all interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 * (This is ONLY used for the idle current measurement by the factory mode.)
 */
int mt_irq_mask_all(struct mtk_irq_mask *mask)
{
	void __iomem *dist_base;

	dist_base = GIC_DIST_BASE;

	if (mask) {
		/*
	#if defined(CONFIG_FIQ_GLUE)
			local_fiq_disable();
	#endif
		*/

		mask->mask0 = readl((dist_base + GIC_DIST_ENABLE_SET));
		mask->mask1 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x4));
		mask->mask2 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x8));
		mask->mask3 = readl((dist_base + GIC_DIST_ENABLE_SET + 0xC));
		mask->mask4 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x10));
		mask->mask5 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x14));
		mask->mask6 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x18));
		mask->mask7 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x1C));
		mask->mask8 = readl((dist_base + GIC_DIST_ENABLE_SET + 0x20));

		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x4));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x8));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0xC));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x10));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x14));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x18));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x1C));
		writel(0xFFFFFFFF, (dist_base + GIC_DIST_ENABLE_CLEAR + 0x20));
		mb();

		/*
	#if defined(CONFIG_FIQ_GLUE)
		local_fiq_enable();
	#endif
		*/
		mask->header = IRQ_MASK_HEADER;
		mask->footer = IRQ_MASK_FOOTER;

		return 0;
	} else {
		return -1;
	}
}

/*
 * mt_irq_mask_restore: restore all interrupts
 * @mask: pointer to struct mtk_irq_mask for storing the original mask value.
 * Return 0 for success; return negative values for failure.
 * (This is ONLY used for the idle current measurement by the factory mode.)
 */
int mt_irq_mask_restore(struct mtk_irq_mask *mask)
{
	void __iomem *dist_base;

	dist_base = GIC_DIST_BASE;

	if (!mask)
		return -1;
	if (mask->header != IRQ_MASK_HEADER)
		return -1;
	if (mask->footer != IRQ_MASK_FOOTER)
		return -1;

	/*
#if defined(CONFIG_FIQ_GLUE)
		  local_fiq_disable();
#endif
	*/

	writel(mask->mask0, (dist_base + GIC_DIST_ENABLE_SET));
	writel(mask->mask1, (dist_base + GIC_DIST_ENABLE_SET + 0x4));
	writel(mask->mask2, (dist_base + GIC_DIST_ENABLE_SET + 0x8));
	writel(mask->mask3, (dist_base + GIC_DIST_ENABLE_SET + 0xC));
	writel(mask->mask4, (dist_base + GIC_DIST_ENABLE_SET + 0x10));
	writel(mask->mask5, (dist_base + GIC_DIST_ENABLE_SET + 0x14));
	writel(mask->mask6, (dist_base + GIC_DIST_ENABLE_SET + 0x18));
	writel(mask->mask7, (dist_base + GIC_DIST_ENABLE_SET + 0x1C));
	writel(mask->mask8, (dist_base + GIC_DIST_ENABLE_SET + 0x20));
	mb();


	/*
#if defined(CONFIG_FIQ_GLUE)
		  local_fiq_enable();
#endif
	*/
	return 0;
}

/*
 * mt_irq_set_pending_for_sleep: pending an interrupt for the sleep manager's use
 * @irq: interrupt id
 * (THIS IS ONLY FOR SLEEP FUNCTION USE. DO NOT USE IT YOURSELF!)
 */
void mt_irq_set_pending_for_sleep(unsigned int irq)
{
	void __iomem *dist_base;
	u32 mask = 1 << (irq % 32);

	dist_base = GIC_DIST_BASE;

	if (irq < 16) {
		pr_err("Fail to set a pending on interrupt %d\n", irq);
		return;
	}

	writel(mask, dist_base + GIC_DIST_PENDING_SET + irq / 32 * 4);
	pr_debug("irq:%d, 0x%p=0x%x\n", irq,
		  dist_base + GIC_DIST_PENDING_SET + irq / 32 * 4, mask);
	mb();
}

u32 mt_irq_get_pending(unsigned int irq)
{
	void __iomem *dist_base;
	u32 bit = 1 << (irq % 32);

	dist_base = GIC_DIST_BASE;

	return (readl_relaxed(dist_base + GIC_DIST_PENDING_SET + irq / 32 * 4) & bit) ? 1 : 0;
}

void mt_irq_set_pending(unsigned int irq)
{
	void __iomem *dist_base;
	u32 bit = 1 << (irq % 32);

	dist_base = GIC_DIST_BASE;
	writel(bit, dist_base + GIC_DIST_PENDING_SET + irq / 32 * 4);
}

/*
 * mt_irq_unmask_for_sleep: enable an interrupt for the sleep manager's use
 * @irq: interrupt id
 * (THIS IS ONLY FOR SLEEP FUNCTION USE. DO NOT USE IT YOURSELF!)
 */
void mt_irq_unmask_for_sleep(unsigned int irq)
{
	void __iomem *dist_base;
	u32 mask = 1 << (irq % 32);

	dist_base = GIC_DIST_BASE;

	if (irq < 16) {
		pr_err("Fail to enable interrupt %d\n", irq);
		return;
	}

	writel(mask, dist_base + GIC_DIST_ENABLE_SET + irq / 32 * 4);
	mb();
}

/*
 * mt_irq_mask_for_sleep: disable an interrupt for the sleep manager's use
 * @irq: interrupt id
 * (THIS IS ONLY FOR SLEEP FUNCTION USE. DO NOT USE IT YOURSELF!)
 */
void mt_irq_mask_for_sleep(unsigned int irq)
{
	void __iomem *dist_base;
	u32 mask = 1 << (irq % 32);

	dist_base = GIC_DIST_BASE;

	if (irq < 16) {
		pr_err("Fail to enable interrupt %d\n", irq);
		return;
	}

	writel(mask, dist_base + GIC_DIST_ENABLE_CLEAR + irq / 32 * 4);
	mb();
}

static int __init mtk_gic_ext_init(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "arm,gic-400");
	if (!node) {
		pr_err("[gic_ext] find arm,gic-400 node failed\n");
		return -EINVAL;
	}

	GIC_DIST_BASE = of_iomap(node, 0);
	if (IS_ERR(GIC_DIST_BASE))
		return -EINVAL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6577-sysirq");
	if (!node) {
		pr_err("[gic_ext] find arm,gic-400 node failed\n");
		return -EINVAL;
	}

	INT_POL_CTL0 = of_iomap(node, 0);
	if (IS_ERR(INT_POL_CTL0))
		return -EINVAL;

	return 0;
}
module_init(mtk_gic_ext_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek gic extend Driver");
MODULE_AUTHOR("Maoguang Meng <maoguang.meng@mediatek.com>");
