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

#include <linux/pci.h>
#include <linux/rwsem.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/pcieport_if.h>

#include "mtk-pci.h"
#include "mtk-pcie.h"
#include "mtk-dma.h"


static int mtk_pci_probe(struct pci_dev *dev, const struct pci_device_id *id);
static void mtk_pci_remove(struct pci_dev *dev);
static void mtk_pci_shutdown(struct pci_dev *dev);
static void mtk_free_irq(struct pci_dev *dev);
static int mtk_setup_msi(struct pci_dev *dev);
static int mtk_setup_msix(struct pci_dev *dev);
static int mtk_setup_intx(struct pci_dev *dev);
static void mtk_cleanup_msix(struct pci_dev *dev);
static void mtk_create_pcie_pme_isr(struct pci_dev *dev);
static void mtk_free_pcie_pme_isr(struct pci_dev *dev);


static const char mtk_name[] = "mtk_bus_driver";
static const char mtk_irq_descr[] = "mtk_pcie_dma";


static const struct mtk_pci_driver mtk_bus_driver = {
	.description =		mtk_name,
	.irq_descr =		mtk_irq_descr,
	.irq =                  pdma_irq,
};

/* PCI driver selection metadata; PCI hotplugging uses this */
static const struct pci_device_id pci_ids[] = { {
		PCI_DEVICE_CLASS(PCI_CLASS_MTK_DEVICE, ~0),
		.driver_data =	(unsigned long) &mtk_bus_driver,
	},
	{ /* end: all zeroes */ }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

/* pci driver glue; this is a "new style" PCI driver module */
static struct pci_driver mtk_pci_driver = {
	.name =		(char *) mtk_name,
	.id_table =	pci_ids,

	.probe =	mtk_pci_probe,
	.remove =	mtk_pci_remove,
	.shutdown =	mtk_pci_shutdown,
};

struct mtk_pci_dev *mtk_create_dev(struct mtk_pci_driver *driver,
	struct device *dev)
{
	struct mtk_pci_dev *mtk_dev;

	mtk_dev = kzalloc(sizeof(*mtk_dev), GFP_KERNEL);
	if (!mtk_dev)
		return NULL;

	mtk_dev->driver = driver;
	dev_set_drvdata(dev, mtk_dev);

	return mtk_dev;
}

struct pci_dev *g_mtk_pci_dev;

int mtk_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct mtk_pci_driver *driver;
	struct mtk_pci_dev *mtk_dev;
	void *buffer;

	xprintk(PCIE_DEBUG, "mtk PCI Probe\n");

	g_mtk_pci_dev = dev;

	driver = (struct mtk_pci_driver *)id->driver_data;
	if (!driver)
		return -1;

	if (pci_enable_device(dev) < 0) {
		xprintk(PCIE_ERR, "pci_enable_device return\n");
		pci_disable_device(dev);
		return -1;
	}

	dev->current_state = PCI_D0;

	mtk_dev = mtk_create_dev(driver, &dev->dev);
	if (!mtk_dev) {
		xprintk(PCIE_ERR, "mtk_create_dev return\n");
		pci_disable_device(dev);
		return -1;
	}

	mtk_dev->irq = dev->irq;
	mtk_dev->rsrc_start = pci_resource_start(dev, 0);
	mtk_dev->rsrc_len = pci_resource_len(dev, 0);

	xprintk(PCIE_ERR, "irq : %d\n", mtk_dev->irq);
	xprintk(PCIE_ERR, "start : 0x%x\n", (unsigned int)mtk_dev->rsrc_start);
	xprintk(PCIE_ERR, "len : %d\n", (unsigned int)mtk_dev->rsrc_len);
	mtk_dev->rsrc_len = 190 * 1024;

	mtk_dev->regs =	ioremap_nocache(mtk_dev->rsrc_start, mtk_dev->rsrc_len);
	mtk_dev->pci_regs = mtk_dev->regs + 0x7000;

	if (mtk_dev->regs == NULL) {
		xprintk(PCIE_ERR, "error mapping memory\n");
		release_mem_region(mtk_dev->rsrc_start, mtk_dev->rsrc_len);
		return -1;
	}

	buffer = kmalloc(DMA_BUFFER_SIZE, GFP_KERNEL | __GFP_DMA);
	mtk_dev->transfer_buffer = buffer;
	mtk_dev->transfer_dma = 0;
	pci_set_master(dev);
	spin_lock_init(&mtk_dev->lock);

	if (request_irq(dev->irq, driver->irq, IRQF_SHARED | IRQF_TRIGGER_LOW,
		driver->irq_descr, mtk_dev)) {
		pr_err("mtk pci_ut, irq register failed!\n");
		iounmap(mtk_dev->regs);
		release_mem_region(mtk_dev->rsrc_start, mtk_dev->rsrc_len);
		pci_disable_device(dev);
		mtk_dev->irq = 0;
		return -1;
	}
	mtk_dev->ep_data_buffer = readl((u8 *)mtk_dev->pci_regs + 0x45c); /* PCIE2AXI_WIN5 */

	xprintk(PCIE_DEBUG, "ep_data_buffer : 0x%lx\n",
		(unsigned long) mtk_dev->ep_data_buffer);
	mtk_create_pcie_pme_isr(dev);

	mtk_create_txrx_ring();

	return 0;
}

void mtk_pci_remove(struct pci_dev *dev)
{
	struct mtk_pci_dev *mtk_dev;
	struct device *pdev;
	void *buffer;
	dma_addr_t mapping;

	xprintk(PCIE_DEBUG, "mtk PCI Remove\n");
	pdev = &dev->dev;

	mtk_dev = pci_get_drvdata(dev);
	if (!mtk_dev)
		return;

	mtk_free_txrx_ring();

	mtk_free_pcie_pme_isr(dev);
	mtk_cleanup_msix(dev);
	iounmap(mtk_dev->regs);
	release_mem_region(mtk_dev->rsrc_start, mtk_dev->rsrc_len);
	buffer = mtk_dev->transfer_buffer;
	mapping = mtk_dev->transfer_dma;
	if (mapping != 0)
		dma_unmap_single(pdev, mapping,
				DMA_BUFFER_SIZE, DMA_BIDIRECTIONAL);

	kfree(buffer);
	kfree(mtk_dev);
	pci_disable_device(dev);
}

void mtk_pci_shutdown(struct pci_dev *dev)
{
	xprintk(PCIE_DEBUG, "mtk PCI Shutdown\n");
}

int mtk_register_pci(void)
{
	return pci_register_driver(&mtk_pci_driver);
}

void mtk_unregister_pci(void)
{
	pci_unregister_driver(&mtk_pci_driver);
}

int mtk_mmio(int type)
{
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;
	u32 reg = 0;

	xprintk(PCIE_DEBUG, "mtk_mmio\n");
	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);

	if (type == MTK_MEM_READ)
		reg = readl((u8 *)mtk_dev->pci_regs + 0x400); /* WDMA_PCIE_ADDR_L */

	if (type == MTK_MEM_WRITE) {
		get_random_bytes(&reg, 4);
		writel(reg, (u8 *)mtk_dev->pci_regs + 0x400);
		/* WDMA_PCIE_ADDR_L_SET_wdma_pciaddr_l(WDMA_PCIE_ADDR_L, reg); */
	}

	xprintk(PCIE_DEBUG, "value : 0x%x\n", reg);

	return 0;
}

int mtk_setup_int(int type)
{
	struct pci_dev *dev;
	int ret = -1;

	xprintk(PCIE_DEBUG, "mtk_setup_int\n");
	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_cleanup_msix(dev);

	if (type == TYPE_MSI)
		ret = mtk_setup_msi(dev);

	if (type == TYPE_MSIX)
		ret = mtk_setup_msix(dev);

	if (type == TYPE_INTx)
		ret = mtk_setup_intx(dev);

	if (ret)
		mtk_cleanup_msix(dev);


	return ret;
}

/*
 * Free IRQs
 * free all IRQs request
 */
static void mtk_free_irq(struct pci_dev *dev)
{
	int i;
	struct mtk_pci_dev *mtk_dev;

	mtk_dev = pci_get_drvdata(dev);
	if (mtk_dev->msix_entries) {
		for (i = 0; i < mtk_dev->msix_count; i++)
			if (mtk_dev->msix_entries[i].vector)
				free_irq(mtk_dev->msix_entries[i].vector,
						 mtk_dev);
	} else if (mtk_dev->irq > 0) {
		free_irq(mtk_dev->irq, mtk_dev);
		mtk_dev->irq = 0;
	}
}

/*
 * Set up MSI
 */
static int mtk_setup_msi(struct pci_dev *dev)
{
	int ret;
	struct mtk_pci_dev	*mtk_dev;
	struct mtk_pci_driver	*driver;

	mtk_dev = pci_get_drvdata(dev);
	driver = mtk_dev->driver;

	ret = pci_enable_msi(dev);
	if (ret) {
		xprintk(PCIE_ERR, "failed to allocate MSI entry\n");
		return ret;
	}

	mtk_dev->irq = dev->irq;
	ret = request_irq(mtk_dev->irq, driver->irq,
		IRQF_SHARED | IRQF_TRIGGER_LOW, driver->irq_descr, mtk_dev);
	if (ret) {
		xprintk(PCIE_ERR, "disable MSI interrupt\n");
		pci_disable_msi(dev);
	}

	return ret;
}

/*
 * Set up MSI-X
 */
static int mtk_setup_msix(struct pci_dev *dev)
{
	int i, ret = 0;
	struct mtk_pci_dev	*mtk_dev;
	struct mtk_pci_driver	*driver;

	mtk_dev = pci_get_drvdata(dev);
	driver = mtk_dev->driver;
	mtk_dev->msix_count = MTK_MSIX_COUNT;
	mtk_dev->msix_entries =
		kmalloc((sizeof(struct msix_entry)) * mtk_dev->msix_count,
				GFP_KERNEL);
	if (!mtk_dev->msix_entries) {
		xprintk(PCIE_ERR, "Failed to allocate MSI-X entries\n");
		return -1;
	}

	for (i = 0; i < mtk_dev->msix_count; i++) {
		mtk_dev->msix_entries[i].entry = i;
		mtk_dev->msix_entries[i].vector = 0;
	}

	ret = pci_enable_msix(dev, mtk_dev->msix_entries, mtk_dev->msix_count);
	if (ret) {
		xprintk(PCIE_ERR, "Failed to enable MSI-X\n");
		goto free_entries;
	}

	for (i = 0; i < mtk_dev->msix_count; i++) {
		ret = request_irq(mtk_dev->msix_entries[i].vector,
					  driver->irq,
					  0, driver->irq_descr, mtk_dev);
		if (ret)
			goto disable_msix;
	}

	return ret;

disable_msix:
	xprintk(PCIE_ERR, "disable MSI-X interrupt\n");
	mtk_free_irq(dev);
	pci_disable_msix(dev);
free_entries:
	kfree(mtk_dev->msix_entries);
	mtk_dev->msix_entries = NULL;
	return ret;
}

/*
 * Set up INTx
 */
static int mtk_setup_intx(struct pci_dev *dev)
{
	struct mtk_pci_dev	*mtk_dev;
	struct mtk_pci_driver	*driver;

	mtk_dev = pci_get_drvdata(dev);
	mtk_dev->irq = dev->irq;
	driver = mtk_dev->driver;

	if ((request_irq(dev->irq, driver->irq, IRQF_SHARED | IRQF_TRIGGER_LOW,
		driver->irq_descr, mtk_dev))) {
		mtk_dev->irq = 0;
		return -1;
	}

	return 0;
}

/* Free any IRQs and disable MSI-X */
static void mtk_cleanup_msix(struct pci_dev *dev)
{
	struct mtk_pci_dev *mtk_dev;

	mtk_dev = pci_get_drvdata(dev);
	mtk_free_irq(dev);

	if (mtk_dev->msix_entries) {
		pci_disable_msix(dev);
		kfree(mtk_dev->msix_entries);
		mtk_dev->msix_entries = NULL;
	} else {
		pci_disable_msi(dev);
	}
}

static void mtk_create_pcie_pme_isr(struct pci_dev *dev)
{
	struct device	*pdev;
	struct pcie_device *srv;
	struct pci_dev *port;
	int ret;

	port = dev->bus->self;
	pdev = &port->dev;
	srv = to_pcie_device(pdev);
	xprintk(PCIE_ERR, "port : 0x%lx\n", (unsigned long)port);
	xprintk(PCIE_ERR, "srv : 0x%lx\n", (unsigned long)srv);
	xprintk(PCIE_ERR, "priv_data : 0x%lx\n", (unsigned long)srv->priv_data);

	if (srv->priv_data != NULL) {
		if (!srv->irq) {
			free_irq(srv->irq, srv);
			kfree(srv->priv_data);
			srv->priv_data = NULL;
			xprintk(PCIE_ERR, "free PME\n");
		}
	}
	if (!port->msi_enabled) {
		ret = pci_enable_msi(port);
		if (ret) {
			xprintk(PCIE_ERR, "failed to allocate MSI entry\n");
			return;
		}
	}
	srv->irq = port->irq;
	xprintk(PCIE_ERR, "srv->irq : %d\n", srv->irq);
	pr_err("dev->irq : %d\n", dev->irq);

	mtk_pme_interrupt_enable(port, false);
	mtk_clear_root_pme_status(port);

	ret = request_irq(srv->irq, mtk_pme_irq,
		IRQF_SHARED, "MTK PCIe PME", dev);
	if (!ret) {
		mtk_pme_interrupt_enable(port, true);
		pr_err("mtk pcie, pme enabled!!\n");
	}
}

static void mtk_free_pcie_pme_isr(struct pci_dev *dev)
{
	struct device	*pdev;
	struct pcie_device *srv;
	struct pci_dev *port;

	port = dev->bus->self;
	pdev = &port->dev;
	srv = to_pcie_device(pdev);
	mtk_pme_interrupt_enable(port, false);
	free_irq(srv->irq, dev);
	pci_disable_msi(port);
}


/*
 *	HOST set own (INT to N9_FW)
 *	HOST write 0x0_41F0[0]=1  (can go to sleep)
 *	0x5000_0014[30]=0x0_4014[30] STS (R/W1C)
 *	0x5000_0014[31]=0x0_4014[31] Enable (RW)
 *	OWN STS : 0x5000_01F0[0] set to 1 (FW)
 *
 * 500001F0[0]=1
 * Host set this bit to transfer ownership to FW.
 * This will introduce an interrupt to FW and 0x014 bit30 will be set to 1.
 * Read this bit will get current ownership.
 * 0: means HOST get ownership. 1: means FW get ownership.
 *
 *
 *
 */
int mtk_set_own(void)
{
#if 0

	UINT32 read_data;

	xprintk(PCIE_ERR, "mtk set own (FW own)\n");
	SET_REG32(CFG_PCIE_LPCR_HOST, PCIE_LPCR_HOST_SET_OWN);
	read_data = READ_REG32(CFG_PCIE_MISC);
	xprintk(PCIE_DEBUG, "0x5000_0014 = %x\n", read_data);
	read_data = READ_REG32(CFG_PCIE_LPCR_HOST);
	xprintk(PCIE_DEBUG, "0x5000_01F0 = %x\n", read_data);
	read_data = READ_REG32(CFG_PCIE_LPCR_FW);
	xprintk(PCIE_DEBUG, "0x5000_01F4 = %x\n", read_data);
	read_data = READ_REG32(MT_INT_SOURCE_CSR);
	xprintk(PCIE_DEBUG, "0x5000_0200 = %x\n", read_data);
	read_data = READ_REG32(MT_INT_MASK_CSR);
	xprintk(PCIE_DEBUG, "0x5000_0204 = %x\n", read_data);
#endif
	return 0;
}

/*
 *	HOST clear own (INT to N9_FW)
 *	HOST write 0x0_41F0[1]=1 (need wakeup)
 *	0x5000_0014[0]=0x0_4014[0] STS (R/W1C)
 *	0x5000_0014[2]=0x0_4014[2] Enable (RW)
 *	OWN STS : 0x5000_01F0[0] no changed
 *
 * 0x5000_01F0[1]=1
 * Host set this bit to request ownership back to HOST from FW.
 * This will introduce an interrupt to FW and 0x014 bit0 will be set to 1.
 * HOST should waiting for INT MESSAGE from FW to indicate the
 * FW is ready from sleep.
 *
 *
 *	FW clear own (INT to HOST)
 *	FW write 0x5000_01F4[0]=1 (ACK of HOST clear own)
 *	0x0_4200[31]=0x5000_0200[31] STS (R/W1C)
 *	0x0_4204[31]=0x5000_0204[31] Enable (RW)
 *	OWN STS : 0x5000_01F4[0] set to 0 (HOST)
 *
 *	FW send INT to HOST (function keep)
 *	FW write 0x5000_0234 (RW)
 *	0x0_4200[30]=0x5000_0200[30] STS (R/W1C)
 *	0x0_4204[30]=0x5000_0204[30] Enable (RW)
 *
 */
int mtk_clear_own(void)
{

	xprintk(PCIE_ERR, "mtk clear own (HOST own)\n");
#if 0
	SET_REG32(CFG_PCIE_LPCR_HOST,  PCIE_LPCR_HOST_CLR_OWN);

	/* enable ISR */
	SET_REG32(MT_INT_MASK_CSR, 1 << 31);
#endif
	return 0;
}

int mtk_pcie_test(void)
{
	pr_err("mtk_pcie_test!!!\n");
	return 0;
}
