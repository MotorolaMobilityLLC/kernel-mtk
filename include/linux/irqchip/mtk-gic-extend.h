/*
 * Copyright (c) 2014 MediaTek Inc.
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

#ifndef __MTK_GIC_EXTEND_H
#define __MTK_GIC_EXTEND_H

#define MT_EDGE_SENSITIVE	0
#define MT_LEVEL_SENSITIVE	1
#define MT_POLARITY_LOW		0
#define MT_POLARITY_HIGH	1

enum {
	IRQ_MASK_HEADER = 0xF1F1F1F1,
	IRQ_MASK_FOOTER = 0xF2F2F2F2
};

struct mtk_irq_mask {
	unsigned int header;	/* for error checking */
	__u32 mask0;
	__u32 mask1;
	__u32 mask2;
	__u32 mask3;
	__u32 mask4;
	__u32 mask5;
	__u32 mask6;
	__u32 mask7;
	__u32 mask8;
	__u32 mask9;
	__u32 mask10;
	__u32 mask11;
	__u32 mask12;
	unsigned int footer;	/* for error checking */
};

void mt_irq_unmask_for_sleep(unsigned int irq);
void mt_irq_mask_for_sleep(unsigned int irq);
int mt_irq_mask_all(struct mtk_irq_mask *mask);
int mt_irq_mask_restore(struct mtk_irq_mask *mask);
void mt_irq_set_pending_for_sleep(unsigned int irq);
extern void mt_irq_set_pending(unsigned int irq);
extern unsigned int mt_irq_get_pending(unsigned int irq);

/* set the priority mask to 0x00 for masking all irqs to this cpu */
void gic_set_primask(void);
/* restore the priority mask value */
void gic_clear_primask(void);
#endif

