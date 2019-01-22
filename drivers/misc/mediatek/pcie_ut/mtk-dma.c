/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/pci-aspm.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include "mtk-pci.h"
#include "mtk-pcie.h"
#include "mtk-dma.h"

#define CONF_REG(r)	(r & 0xFFC)
#define CONF_BUS(b)	((b & 0xFF) << 24)
#define CONF_DEV(d)	((d & 0x1F) << 19)
#define CONF_FUNC(f)	((f & 0x7) << 16)

u32 irqStatus;

int compare_packet1(void *buf1, void *buf2, int len)
{
	void *ptr1;
	void *ptr2;
	u32 length;
	u32 i;

	ptr1 = buf1;
	ptr2 = buf2;
	length = len;

	for (i = 0;  i < length; i++) {
		if ((*(u8 *)ptr2) != (*(u8 *)ptr1)) {
			xprintk(PCIE_ERR, "[loopback data(%d) : 0x%x] != [source data(%d) : 0x%x]\n",
					i, *(u8 *)ptr2, i, *(u8 *)ptr1);
			return -1;
		}
		(u8 *)ptr1++;
		(u8 *)ptr2++;
	}
	return 0;
}

irqreturn_t pdma_irq(int irq, void *__res)
{
	unsigned long           flags;
	irqreturn_t               rc;
	struct mtk_pci_dev   *mtk_dev = __res;
	u32                   IntSource;

	spin_lock_irqsave(&mtk_dev->lock, flags);

	IntSource = READ_REG32(MT_INT_SOURCE_CSR);
	if (IntSource == 0xffffffff)
		xprintk(PCIE_ERR, "pdma_irq fail : IntSource=0x%x\n", IntSource);


	if (!IntSource) {

	} else {

		if (IntSource & MT_INT_T0_DONE) {
			RESET_REG32(MT_INT_MASK_CSR, MT_INT_T0_DONE);
			irqStatus |= (1 << 0);

			SET_REG32(MT_INT_MASK_CSR, MT_INT_T0_DONE);
			complete(&mtk_dev->tx_done);
		}

		if (IntSource & MT_INT_R0_DONE)  {
			RESET_REG32(MT_INT_MASK_CSR, MT_INT_R0_DONE);
			irqStatus |= (1 << 1);

			SET_REG32(MT_INT_MASK_CSR, MT_INT_R0_DONE);
			complete(&mtk_dev->rx_done);
		}

		WRITE_REG32(MT_INT_SOURCE_CSR, IntSource);

	}

	rc = IRQ_HANDLED;

	spin_unlock_irqrestore(&mtk_dev->lock, flags);

	return rc;
}


