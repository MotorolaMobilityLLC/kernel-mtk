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

#ifndef _MTK_PCI_
#define _MTK_PCI_

#include <linux/irq.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>

#include "common.h"

extern struct pci_dev *g_mtk_pci_dev;
extern int mtk_pcie_pe_reset(struct pci_dev *dev);
extern int mtk_pcie_warm_reset(struct pci_dev *dev, int pe_rst);

#define PKT_HEADER_SIZE	8  /* Packet Header[8] = Signature[4] + packet length[2] + dma mode[1] + data pattern[1] */
#define TXD_SIZE	16 /* 4dwords */
#define RXD_SIZE	16 /* 4dwords */

#define TX_RING_SIZE    8  /* Total DMAD in a ring */
#define RX_RING_SIZE    8  /* Total DMAD in a ring */

#define MT_INT_T0_DONE  BIT(4)
#define MT_INT_R0_DONE  BIT(0)



struct _TEST_PKT_T {
	u8		ucFixedHdr[4];
	u16		packet_Length;
	u8		dma_mode;
	u8		data_pattern;
	u8		ucPayload[];
};


struct mtk_pci_driver {
	const char	*description;
	const char	*irq_descr;
	/* irq handler */
	irqreturn_t	(*irq)(int irq, void *__res);
	int		*irq_buf;
};

struct mtk_pci_dev {
	int			irq;		/* irq allocated */
	void __iomem		*regs;		/* device memory/io */
	void __iomem		*pci_regs;
	u64			rsrc_start;	/* memory/io resource start */
	u64			rsrc_len;	/* memory/io resource length */
	unsigned		power_budget;	/* in mA, 0 = no limit */
	spinlock_t		lock;

	/* msi-x vectors */
	int			msix_count;
	struct msix_entry	*msix_entries;

	/* (in) associated data buffer */
	void			*transfer_buffer;

	dma_addr_t	transfer_dma;	/* (in) dma addr for transfer_buffer */
	u32			transfer_data_length;	/* (in) data buffer length */
	dma_addr_t		ep_data_buffer;	/* (in) EP data buffer for loopback*/
	u32		actual_length;	/* (return) actual transfer length */
	u32		dma_done;	/* (return) dma status */
	u32			buffer_offset;	/* dma buffer offset*/
	u32		pme_event;	/* (return) receive pme event */
	struct mtk_pci_driver	*driver;

	/* tx/rx descriptor */
	dma_addr_t tx_desc_phy;
	dma_addr_t rx_desc_phy;
	u8 *tx_desc_buffer;
	u8 *rx_desc_buffer;

	/* tx/rx ring buffer */
	dma_addr_t tx_ring_phy;
	dma_addr_t rx_ring_phy;
	u8 *tx_ring_buffer;
	u8 *rx_ring_buffer;

	struct completion tx_done;
	struct completion rx_done;
};

#define PCI_CLASS_MTK_DEVICE	0x000280
#define PCI_BAR_NUM		6
#define MTK_MSIX_COUNT		2
#define MTK_MSIX_COUNT		2
#define DMA_BUFFER_SIZE		0x1000
#define PCIE_DMA_TIMEOUT		(5*HZ)
#define PCIE_TX_done_TIMEOUT		(5*HZ)
#define PCIE_RX_done_TIMEOUT		(5*HZ)
#define LINK_RETRAIN_TIMEOUT		(HZ)
#define L1_REMOTE_WAKEUP_TIMEOUT	(10*HZ)
#define L2_REMOTE_WAKEUP_TIMEOUT	(60*HZ)


#define PCIE_EMERG		(1<<0)	/* system is unusable			*/
#define PCIE_ALERT		(1<<1)	/* action must be taken immediately	*/
#define PCIE_CRIT		(1<<2)	/* critical conditions			*/
#define PCIE_ERR		(1<<3)	/* error conditions			*/
#define PCIE_WARNING		(1<<4)	/* warning conditions			*/
#define PCIE_NOTICE		(1<<5)	/* normal but significant condition	*/
#define PCIE_INFO		(1<<6)	/* informational			*/
#define PCIE_DEBUG		(1<<7)	/* debug-level messages			*/


#define DebugLevel 0xF
#define DebugDisable 0
#define GetDebugLevel()	(DebugLevel)
#define _dbg_level(level) (!DebugDisable && (level >= -1 && GetDebugLevel() >= level))
#define xprintk(level, format, args...) do {\
	if (_dbg_level(level)) { \
		printk(format, ##args); \
	} } while (0)


extern int mtk_register_pci(void);
extern void mtk_unregister_pci(void);
extern int mtk_mmio(int type);
extern int mtk_setup_int(int type);

#endif
