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
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/pci-aspm.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>
#include <linux/kthread.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/suspend.h>
#include <linux/rtc.h>

#include "mtk-pci.h"
#include "mtk-pcie.h"
#include "mtk-dma.h"

#define DMA_PCI_mode 0

struct pci_bus *rescan_bus;

struct pci_dev	*g_dev;
struct pci_dev	*dev;
struct device	*pdev;
struct mtk_pci_dev  *mtk_dev;

struct pci_dev *mtk_get_pcie_resource(int class_code)
{
	return g_mtk_pci_dev;
}

void mtk_clean_resource(void)
{
	g_mtk_pci_dev = NULL;
}


int mtk_pci_check_bar0(struct pci_dev *dev)
{
	struct pci_bus	*bus;
	unsigned int	devfn, l;

	bus = dev->bus;
	devfn = dev->devfn;
	pci_bus_read_config_dword(bus, devfn, PCI_BASE_ADDRESS_0, &l);
	if (l & 0xFFFFFFF0)
		return -1;

	return 0;
}

int mtk_pci_retrain(struct pci_dev *dev)
{
	u16 reg16;
	struct pci_dev *parent;
	int pos, ppos;
	unsigned long start;

	parent = dev->bus->self;
	ppos = parent->pcie_cap;
	pos = dev->pcie_cap;
	/* Retrain link */
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL, &reg16);
	reg16 |= PCI_EXP_LNKCTL_RL;
	pci_write_config_word(parent, ppos + PCI_EXP_LNKCTL, reg16);
	/* Wait for link training end. Break out after waiting for timeout */
	start = jiffies;
	msleep(20);
	for (;;) {
		pci_read_config_word(parent, ppos + PCI_EXP_LNKSTA, &reg16);

		if (!(reg16 & PCI_EXP_LNKSTA_LT))
			return 0;

		if (time_after(jiffies, start + LINK_RETRAIN_TIMEOUT))
			return -1;

		msleep(20);
	}
	return 0;
}

int mtk_pci_get_retrain_status(struct pci_dev *dev)
{
	u16 reg16;
	struct pci_dev *parent;
	int ppos;
	unsigned long start = jiffies;

	parent = dev->bus->self;
	ppos = parent->pcie_cap;

	for (;;) {
		pci_read_config_word(parent, ppos + PCI_EXP_LNKSTA, &reg16);
		if (!(reg16 & PCI_EXP_LNKSTA_LT))
			return 0;

		if (time_after(jiffies, start + LINK_RETRAIN_TIMEOUT))
			return -1;

		msleep(20);
	}

	return 0;
}

int mtk_pci_link_disabled(struct pci_dev *dev)
{
	u16 reg16;
	struct pci_dev *parent;
	int ppos;

	parent = dev->bus->self;
	ppos = parent->pcie_cap;

	/* disable link */
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL, &reg16);
	reg16 |= PCI_EXP_LNKCTL_LD;
	pci_write_config_word(parent, ppos + PCI_EXP_LNKCTL, reg16);

	return 0;
}

int mtk_pci_link_enabled(struct pci_dev *dev)
{
	u16 reg16;
	struct pci_dev *parent;
	int ppos;

	parent = dev->bus->self;
	ppos = parent->pcie_cap;

	/* enable link */
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL, &reg16);
	reg16 &= ~PCI_EXP_LNKCTL_LD;
	pci_write_config_word(parent, ppos + PCI_EXP_LNKCTL, reg16);

	return 0;
}

static void mtk_pcie_config_aspm_dev(struct pci_dev *dev, int val)
{
	u16 reg16;
	int pos = dev->pcie_cap;

	pr_debug("(config aspm) dev : 0x%p, value : 0x%x\n", dev, val);

	pci_read_config_word(dev, pos + PCI_EXP_LNKCTL, &reg16);
	reg16 &= ~0x3;
	reg16 |= val;
	pci_write_config_word(dev, pos + PCI_EXP_LNKCTL, reg16);
}

int mtk_set_mps_mrrs(int size)
{
	struct pci_dev *dev;
	int i, val;
	int pos;
	u32 cap;
	u16 control;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pos = dev->pcie_cap;
	pci_read_config_dword(dev, pos + PCI_EXP_DEVCAP, &cap);
	cap &= PCI_EXP_DEVCAP_PAYLOAD;
	i = size;
	i >>= 7;
	val = 0;
	do {
		i >>= 1;
		val++;
	} while (i > 0);
	val -= 1;

	if (val > cap) {
		pr_err("not support this payload size\n");
		return -1;
	}

	pci_read_config_word(dev, pos + PCI_EXP_DEVCTL, &control);
	control &= ~PCI_EXP_DEVCTL_PAYLOAD;
	control |= val << 5;
	control &= ~PCI_EXP_DEVCTL_READRQ;
	control |= val << 12;
	pci_write_config_word(dev, pos + PCI_EXP_DEVCTL, control);

	return 0;
}

struct pci_dev *rc_address;
struct pci_dev *ep_address;

int mtk_pcie_aspm(struct mkt_aspm_state aspm)
{
	struct pci_dev *child, *parent;

	child = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!child) {
		mtk_pcie_config_aspm_dev(
			(struct pci_dev *)rc_address, aspm.aspm_ds);
		return -1;
	}
	parent = child->bus->self;
	rc_address = parent;

	mtk_pcie_config_aspm_dev(child, aspm.aspm_us);
	mtk_pcie_config_aspm_dev(parent, aspm.aspm_ds);

	return 0;
}

int mtk_pcie_retrain(void)
{
	struct pci_dev *dev;
	int ret;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	ret = mtk_pci_retrain(dev);

	return ret;
}

static int mtk_pcie_check_speed(struct pci_dev *dev)
{
	int ret = 0;
	int pos, ppos;
	u16 plinksta, plinkctl2;
	struct pci_dev *parent;

	pos = dev->pcie_cap;
	parent = dev->bus->self;
	ppos = parent->pcie_cap;
	pci_read_config_word(parent, ppos + PCI_EXP_LNKSTA, &plinksta);
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL2, &plinkctl2);
	if ((plinksta & PCI_EXP_LNKSTA_CLS) != (plinkctl2 & PCI_SPEED_MASK))
		ret = 1;
	else
		ret = 0;

	return ret;
}

int mtk_pcie_disabled(void)
{
	struct pci_dev *dev;
	int ret;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_save_state(dev);
	ret = mtk_pci_link_disabled(dev);
	msleep(100);
	ret = mtk_pci_link_enabled(dev);
	msleep(100);

	/* check if device has been reset */
	ret = mtk_pci_check_bar0(dev);
	if (ret) {
		pr_err("warm reset failed!\n");
		return ret;
	}
	pci_restore_state(dev);

	/* check if the speed needs to train to 5G or not */
	if (mtk_pcie_check_speed(dev)) {
		ret = mtk_pci_retrain(dev);
		if (ret) {
			pr_debug("retrain failed!\n");
			return ret;
		}
	}
	return ret;
}

static int mtk_pcie_flr(struct pci_dev *dev)
{
	int i;
	int pos;
	u32 cap;
	u16 status, control;

	pos = dev->pcie_cap;
	pci_read_config_dword(dev, pos + PCI_EXP_DEVCAP, &cap);
	if (!(cap & PCI_EXP_DEVCAP_FLR)) {
		pr_err("Function Level Reset not support!!!\n");
		return -1;
	}
	/* Wait for Transaction Pending bit clean */
	for (i = 0; i < 4; i++) {
		if (i)
			msleep((1 << (i - 1)) * 100);

		pci_read_config_word(dev, pos + PCI_EXP_DEVSTA, &status);
		if (!(status & PCI_EXP_DEVSTA_TRPND))
			goto clear;
	}

clear:
	pci_read_config_word(dev, pos + PCI_EXP_DEVCTL, &control);
	control |= PCI_EXP_DEVCTL_BCR_FLR;
	pci_write_config_word(dev, pos + PCI_EXP_DEVCTL, control);

	msleep(100);

	return 0;
}

int mtk_function_level_reset(void)
{
	struct pci_dev *dev;


	int ret = 0;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_save_state(dev);
	ret = mtk_pcie_flr(dev);

	/* check if device has been reset */
	ret = mtk_pci_check_bar0(dev);
	if (ret) {
		pr_err("function level reset failed!\n");
		return ret;
	}
	pci_restore_state(dev);

	return ret;

}

int mtk_pci_parent_bus_reset(struct pci_dev *dev)
{
	u16 ctrl;
	struct pci_dev *parent;
	int pos, ppos;

	pr_debug("mtk_pci_parent_bus_reset\n");

	pos = dev->pcie_cap;
	/* disable Enter Compliance bit */
	pci_read_config_word(dev, pos + PCI_EXP_LNKCTL2, &ctrl);
	ctrl &= ~0x10;
	pci_write_config_word(dev, pos + PCI_EXP_LNKCTL2, ctrl);
	parent = dev->bus->self;
	ppos = parent->pcie_cap;
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL2, &ctrl);
	ctrl &= ~0x10;
	pci_write_config_word(parent, ppos + PCI_EXP_LNKCTL2, ctrl);
	pci_read_config_word(parent, PCI_BRIDGE_CONTROL, &ctrl);
	ctrl |= PCI_BRIDGE_CTL_BUS_RESET;
	pci_write_config_word(parent, PCI_BRIDGE_CONTROL, ctrl);
	msleep(20);
	ctrl &= ~PCI_BRIDGE_CTL_BUS_RESET;
	pci_write_config_word(parent, PCI_BRIDGE_CONTROL, ctrl);
	msleep(100);

	return 0;

}


int mtk_pcie_hot_reset(void)
{
	struct pci_dev *dev;
	int ret;

	pr_debug("mtk_pcie_hot_reset\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_save_state(dev);
	ret = mtk_pci_parent_bus_reset(dev);
	ret = mtk_pci_check_bar0(dev);
	if (ret) {
		pr_err("hot reset failed!\n");
		return ret;
	}
	pci_restore_state(dev);

	/* check if the speed needs to train to 5G or not */
	if (mtk_pcie_check_speed(dev)) {
		ret = mtk_pci_retrain(dev);
		if (ret) {
			pr_err("retrain failed!\n");
			return ret;
		}
	}
	return ret;

}

int mtk_pcie_ut_warm_reset(void)
{
	struct pci_dev *dev;
	int ret = -1, err = 0;

	pr_debug("mtk_pcie_warm_reset\n");
	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_save_state(dev);
	ret = mtk_pcie_warm_reset(dev, 1);
	if (ret) {
		pr_err("warm reset, toggle pe_reset failed!\n");
		err = 1;
		goto restore;
	}

	/* check if device has been reset */
	ret = mtk_pci_check_bar0(dev);
	if (ret) {
		pr_err("warm reset check bar0 failed!\n");
		err = 1;
		goto restore;
	}
restore:
	pci_restore_state(dev);

	/* check if the speed needs to train to 5G or not */
	if (mtk_pcie_check_speed(dev)) {
		ret = mtk_pci_retrain(dev);
		if (ret) {
			pr_debug("retrain failed!\n");
			return ret;
		}
	}
	return (ret | err);

}

static int mtk_pcie_speed(struct pci_dev *dev, int speed)
{
	int ret;
	int pos, ppos;
	u16 linksta, plinksta, plinkctl2;
	struct pci_dev *parent;

	pos = dev->pcie_cap;
	pci_read_config_word(dev, pos + PCI_EXP_LNKSTA, &linksta);
	parent = dev->bus->self;
	ppos = parent->pcie_cap;

	pci_read_config_word(parent, ppos + PCI_EXP_LNKSTA, &plinksta);
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL2, &plinkctl2);
	if ((plinkctl2 & PCI_SPEED_MASK) != speed) {
		plinkctl2 &= ~PCI_SPEED_MASK;
		plinkctl2 |= speed;
		pci_write_config_word(parent,
			ppos + PCI_EXP_LNKCTL2, plinkctl2);
	}
	if (((linksta & PCI_EXP_LNKSTA_CLS) == speed) &&
		((plinksta & PCI_EXP_LNKSTA_CLS) == speed))
		return 0;

	ret = mtk_pcie_retrain();
	if (ret) {
		pr_err("retrain fails\n");
		return ret;
	}
	parent = dev->bus->self;
	pos = parent->pcie_cap;
	pci_read_config_word(parent, pos + PCI_EXP_LNKSTA, &linksta);

	if ((linksta & PCI_EXP_LNKSTA_CLS) != speed) {
		pr_err("change speed failed!! linksta = 0x%x\n", linksta);
		return -1;
	}

	return 0;
}

int mtk_pcie_change_speed(int speed)
{
	struct pci_dev *dev;
	int ret;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev) {
		pr_err("can't find target device\n");
		return -1;
	}

	/* train speed */
	ret = mtk_pcie_speed(dev, speed);

	return ret;
}

int mtk_pcie_configuration(void)
{
	struct pci_dev *dev;
	int l, devfn;
	struct pci_bus	*bus;
	unsigned int pos, nonce;
	int ret;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev) {
		pr_err("can't find target device\n");
		return -1;
	}

	bus = dev->bus;
	devfn = dev->devfn;

	pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &l);
	if ((l & 0xffff) != dev->vendor) {
		pr_err("read vendor id error!![0x%x != 0x%x]\n",
						(l & 0xffff), dev->vendor);
		return -1;
	}
	if (((l >> 16) & 0xffff) != dev->device) {
		pr_err("read device id error!![0x%x != 0x%x]\n",
					((l >> 16) & 0xffff), dev->device);
		return -1;
	}
	dev->vendor = l & 0xffff;
	dev->device = (l >> 16) & 0xffff;

	pci_bus_read_config_dword(bus, devfn, PCI_CLASS_REVISION, &l);
	if ((l & 0xff) != dev->revision) {
		pr_err("read revision id error!![0x%x != 0x%x]\n",
					(l & 0xffff), dev->revision);
		return -1;
	}
	if ((l >> 8) != dev->class) {
		pr_err("read class id error!![0x%x != 0x%x]\n",
					((l >> 8) & 0xffff), dev->class);
		return -1;
	}

	pci_save_state(dev);

	for (pos = 0; pos < 2; pos++) {

		get_random_bytes(&nonce, 4);

		ret = pci_bus_write_config_dword(bus, devfn,
				PCI_BASE_ADDRESS_0 + (pos << 2), nonce);
		if (ret) {
			pr_err("write config failed at baseaddr %x\n",
				PCI_BASE_ADDRESS_0 + (pos << 2));
			return -1;
		}

		ret = pci_bus_read_config_dword(bus, devfn,
				PCI_BASE_ADDRESS_0 + (pos << 2), &l);
		if (ret) {
			pr_err("read config failed at baseaddr %x\n",
				PCI_BASE_ADDRESS_0 + (pos << 2));
			return -1;
		}

		if ((nonce & 0xfff00000) != (l & 0xfff00000)) {
			pr_err("config r/w compare fail at baseaddr=%x, write=%x, read=%x\n",
							PCI_BASE_ADDRESS_0 + (pos << 2), nonce, l);
			return -1;
		}
	}

	pci_restore_state(dev);

	return 0;
}

int mtk_pcie_link_width(void)
{
	struct pci_dev *dev;
	unsigned int pos, reg32, num, i, l, devfn;
	int ret;
	u16 reg16;
	struct pci_bus	*bus;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;


	pos = dev->pcie_cap;
	pci_read_config_dword(dev, pos + PCI_EXP_LNKCAP, &reg32);
	num = reg32;

	for (i = 0; i < num; i++) {
		/* Select Lane */


		ret = mtk_pci_retrain(dev);
		if (ret)
			return ret;


		/* Nogotiated Link Width */
		pci_read_config_word(dev, pos + PCI_EXP_LNKSTA, &reg16);
		if ((reg16 & PCI_EXP_LNKSTA_NLW) != i)
			return ret;

		/* Configuration Read Validation */
		bus = dev->bus;
		devfn = dev->devfn;
		pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &l);
		if ((l & 0xffff) != dev->vendor) {
			pr_err("read vendor id error!![0x%x != 0x%x]\n",
				(l & 0xffff), dev->vendor);
			return -1;
		}
		if (((l >> 16) & 0xffff) != dev->device) {
			pr_err("read device id error!![0x%x != 0x%x]\n",
				((l >> 16) & 0xffff), dev->device);
			return -1;
		}
	}
	return ret;
}

int mtk_pcie_reset_config(void)
{
	struct pci_dev *dev, *parent;
	unsigned int pos, l, devfn, ppos;
	int ret;
	u16 reg16, plinksta;
	struct pci_bus	*bus;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_save_state(dev);
	ret = mtk_pci_parent_bus_reset(dev);
	pci_restore_state(dev);
	if (ret)
		return ret;


	pos = dev->pcie_cap;
	parent = dev->bus->self;
	ppos = parent->pcie_cap;
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL2, &plinksta);
	if ((plinksta & PCI_SPEED_MASK) == MTK_SPEED_5_0GT) {
		ret = mtk_pcie_retrain();
		if (ret) {
			pr_err("retrain fails\n");
			return ret;
		}
	}
	bus = dev->bus;
	devfn = dev->devfn;
	pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &l);
	if ((l & 0xffff) != dev->vendor) {
		pr_err("read vendor id error!![0x%x != 0x%x]\n",
			(l & 0xffff), dev->vendor);
		return -1;
	}
	pci_read_config_word(dev, pos + PCI_EXP_LNKCTL, &reg16);
	reg16 &= ~0x3;
	reg16 |= LINK_STATE_L1;
	pci_write_config_word(dev, pos + PCI_EXP_LNKCTL, reg16);
	pci_read_config_word(dev, pos + PCI_EXP_LNKCTL, &reg16);
	if (!(reg16 & LINK_STATE_L1)) {
		pr_err("configuration write error\n");
		return -1;
	}
	reg16 &= ~0x3;
	pci_write_config_word(dev, pos + PCI_EXP_LNKCTL, reg16);

	return 0;
}
#if 0
dma_addr_t dma_buffer_map(struct pci_dev *dev)
{
	struct device	*pdev;
	struct mtk_pci_dev	*mtk_dev;
	dma_addr_t mapping;
	void *buffer;

	pdev = &dev->dev;
	mtk_dev = pci_get_drvdata(dev);
	buffer = mtk_dev->transfer_buffer;
	mapping = dma_map_single(pdev, buffer, DMA_BUFFER_SIZE,
							DMA_BIDIRECTIONAL);
	dma_sync_single_for_device(pdev, mapping, DMA_BUFFER_SIZE,
							DMA_BIDIRECTIONAL);
	mtk_dev->transfer_dma = mapping;

	return mapping;
}

void dma_buffer_unmap(struct pci_dev *dev)
{
	struct device	*pdev;
	struct mtk_pci_dev	*mtk_dev;
	dma_addr_t mapping;
	void *buffer;

	pdev = &dev->dev;
	mtk_dev = pci_get_drvdata(dev);
	buffer = mtk_dev->transfer_buffer;
	mapping = mtk_dev->transfer_dma;
	dma_unmap_single(pdev, mapping, DMA_BUFFER_SIZE, DMA_BIDIRECTIONAL);
}

void fill_dma_data(void *buf, int nbytes, int seed)
{
	int i;

	for (i = 0; i < ((nbytes / 4) + 1); i++)
		*((int *)(buf + 4 * i)) = seed + i;

}

int mtk_pcie_prepare_dma(int type, int length)
{
	struct pci_dev *dev;
	struct mtk_pci_dev	*mtk_dev;
	int addr;
	unsigned char ran;
	void *buffer;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);
	buffer = mtk_dev->transfer_buffer;
	mtk_dev->transfer_data_length = length;
	mtk_dev->actual_length = 0;
	mtk_dev->dma_done = 0;

	if (type == MTK_MEM_WRITE) {

		mtk_dev->buffer_offset = (int_count % 4);
		get_random_bytes(&ran, 1);
		if ((ran == 0) || (ran == 0xFF))
			memset(buffer, ran, length);
		else
			fill_dma_data(buffer, length, ran);
		int_count++;
	}
	addr = dma_buffer_map(dev);
	if (!addr)
		return -1;

	return 0;
}

int save_dma_tx_data(void *buf)
{
	struct pci_dev *dev;
	struct mtk_pci_dev	*mtk_dev;
	void *src;
	int size;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);
	src = mtk_dev->transfer_buffer;
	size = mtk_dev->transfer_data_length;
	memcpy(buf, src, size);

	return 0;
}

int compare_dma_data(void *buf)
{
	struct pci_dev *dev;
	struct mtk_pci_dev	*mtk_dev;
	int *ptr1, *ptr2, length, i;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;
	ptr1 = buf;
	mtk_dev = pci_get_drvdata(dev);
	ptr2 = mtk_dev->transfer_buffer;
	length = mtk_dev->transfer_data_length;
	length >>= 2;

	for (i = 0;  i < length; i++) {
		if (*(ptr2 + i) != *(ptr1 + i)) {
			pr_err("[loopback data(%d) : 0x%x] != [source data(%d) : 0x%x]\n",
							i, *(ptr2 + i), i, *(ptr1 + i));
			return -1;
		}
	}
	return 0;
}

int mtk_pcie_dma_end(int type)
{
	struct pci_dev *dev;

	pr_debug("mtk_pcie_dma_end\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	dma_buffer_unmap(dev);

	if (type == MTK_MEM_WRITE)
		mtk_pcie_free_wdma();

	if (type == MTK_MEM_READ)
		mtk_pcie_free_rdma();

	return 0;
}

int wait_dma_done(void)
{
	unsigned long start;
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;
	int *done;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;
	mtk_dev = pci_get_drvdata(dev);
	done = (int *) &mtk_dev->dma_done;
	start = jiffies;
	for (;;) {
		if (*done) {
			*done = 0;
			return 0;
		} else if (time_after(jiffies, start + PCIE_DMA_TIMEOUT))
			return -1;

		msleep(20);
	}

	return 0;
}

int mtk_pcie_memory(int type)
{
	struct pci_dev *dev;
	int count;
	struct mtk_pci_dev	*mtk_dev;
	int addr, bus_addr, actual, offset;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);
	count = mtk_dev->transfer_data_length - mtk_dev->actual_length;
	count = (count > MAX_DMA_LENGTH) ? MAX_DMA_LENGTH : count;
	actual = mtk_dev->actual_length;
	offset = (((actual % DMA_BUFFER_SIZE) + count) > DMA_BUFFER_SIZE) ?
						0 : (actual % DMA_BUFFER_SIZE);
	addr = mtk_dev->transfer_dma + offset;
	bus_addr = mtk_dev->ep_data_buffer + mtk_dev->buffer_offset + offset;

	pr_debug("transfer_data_length : 0x%x\n", mtk_dev->transfer_data_length);
	pr_debug("actual : 0x%x\n", actual);
	pr_debug("count : 0x%x\n", count);
	pr_debug("transfer_dma : 0x%llx\n", mtk_dev->transfer_dma);
	pr_debug("ep_data_buffer : 0x%llx\n", mtk_dev->ep_data_buffer);
	pr_debug("addr : 0x%x\n", addr);
	pr_debug("bus_addr : 0x%x\n", bus_addr);

	mtk_pcie_config_dma(TRUE, type, addr, bus_addr, count);

	return 0;
}
#endif
int mtk_pcie_go_l1(void)
{
	struct pci_dev *dev;
	u16 pmcsr;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;
	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
	pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
	pmcsr |= PCI_D3hot;
	pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL, pmcsr);

	return 0;
}

int mtk_pcie_l2(int exit)
{
	struct pci_dev *dev;
	int ret, mpcie;
	struct pci_dev *parent;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mpcie = (pci_find_ext_capability(dev, PCI_EXT_CAP_ID_MPCIE)) ? 1 : 0;
	parent = dev->bus->self;
	if (mpcie) {
		pr_err("mpcie is not supported!!!\n");
		return -1;
	}
	pci_save_state(dev);
	pci_save_state(parent);

	ret = mtk_pcie_enter_l2(dev);
	if (ret) {
		pr_err("mtk_pcie_enter_l2 failed!!\n");
		return ret;
	}

	ret = mtk_pcie_exit_l2(dev, exit);
	if (ret) {
		pr_err("mtk_pcie_exit_l2 failed!!\n");
		return ret;
	}

	parent = dev->bus->self;

	pci_restore_state(parent);
	pci_restore_state(dev);

	/* check if the speed needs to train to 5G or not */
	if (mtk_pcie_check_speed(dev)) {
		ret = mtk_pci_retrain(dev);
		if (ret) {
			pr_err("retrain failed!\n");
			return ret;
		}
	}

	return 0;
}

int mtk_pcie_wakeup(void)
{

	struct pci_dev *dev;
	u16 pmcsr;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
	pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
	pmcsr |= PCI_D0;
	pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL, pmcsr);

	return 0;
}

int mtk_pcie_wait_l2_exit(struct pci_dev *dev)
{

	unsigned long start;
	int ltssm;
	int ret;
	struct pci_dev *parent;

	parent = dev->bus->self;

	start = jiffies;
	for (;;) {

		ltssm = READ_REGISTER_UINT32(MTK_TEST_OUT_00);

		mdelay(1);
		if ((ltssm & ltssm_mask) == state_L0)
			break;
		else if (time_after(jiffies, start + L2_REMOTE_WAKEUP_TIMEOUT))
			return -1;

	}
	pci_restore_state(parent);
	pci_restore_state(dev);

	/* check if the speed needs to train to 5G or not */
	if (mtk_pcie_check_speed(dev)) {
		ret = mtk_pci_retrain(dev);
		if (ret) {
			pr_err("retrain failed!\n");
			return ret;
		}
	}

	return 0;
}

void mtk_enable_pme(void)
{
	struct pci_dev *dev;
	u16 pmcsr;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return;

	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);

	/* Clear PME_Status by writing 1 to it and enable PME# */
	pmcsr |= PCI_PM_CTRL_PME_STATUS | PCI_PM_CTRL_PME_ENABLE;
	pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL, pmcsr);
}

int mtk_wait_wakeup(void)
{
	struct pci_dev *dev;
	unsigned long start;
	struct mtk_pci_dev	*mtk_dev;
	u32 *pme_event;
	int status;
	u16 pmcsr;

	status = 0;
	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);
	pme_event = (u32 *)&mtk_dev->pme_event;
	start = jiffies;
	for (;;) {
		if (*pme_event) {
			*pme_event = 0;
			status = 0;
			break;
		} else if (time_after(jiffies, start + L1_REMOTE_WAKEUP_TIMEOUT)) {
			status = -1;
			break;
		}
		msleep(20);
	}
	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
	pmcsr &= ~PCI_PM_CTRL_PME_ENABLE;
	if (pmcsr & PCI_PM_CTRL_PME_STATUS)
		pmcsr |= PCI_PM_CTRL_PME_STATUS;

	pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL, pmcsr);
	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
	mtk_pcie_wakeup();

	return status;
}

void mtk_pme_interrupt_enable(struct pci_dev *dev, bool enable)
{
	int rtctl_pos;
	u16 rtctl;

	rtctl_pos = dev->pcie_cap + PCI_EXP_RTCTL;

	pci_read_config_word(dev, rtctl_pos, &rtctl);
	if (enable)
		rtctl |= PCI_EXP_RTCTL_PMEIE;
	else
		rtctl &= ~PCI_EXP_RTCTL_PMEIE;

	pci_write_config_word(dev, rtctl_pos, rtctl);
}

u32 mtk_get_root_pme_status(struct pci_dev *dev)
{
	int rtsta_pos;
	u32 rtsta;

	rtsta_pos = dev->pcie_cap + PCI_EXP_RTSTA;

	pci_read_config_dword(dev, rtsta_pos, &rtsta);
	rtsta &= PCI_EXP_RTSTA_PME;

	return rtsta;
}

void mtk_clear_root_pme_status(struct pci_dev *dev)
{
	int rtsta_pos;
	u32 rtsta;

	rtsta_pos = dev->pcie_cap + PCI_EXP_RTSTA;

	pci_read_config_dword(dev, rtsta_pos, &rtsta);
	rtsta |= PCI_EXP_RTSTA_PME;
	pci_write_config_dword(dev, rtsta_pos, rtsta);
}

irqreturn_t mtk_pme_irq(int irq, void *__res)
{
	unsigned long		flags;
	struct pci_dev *dev = __res;
	struct pci_dev *port;
	struct mtk_pci_dev	*mtk_dev;

	pr_err("mtk_pme_irq got PME#!\n");
	mtk_dev = pci_get_drvdata(dev);
	spin_lock_irqsave(&mtk_dev->lock, flags);
	port = dev->bus->self;
	if (!mtk_get_root_pme_status(port)) {

		spin_unlock_irqrestore(&mtk_dev->lock, flags);
		return IRQ_HANDLED;
	}
	mtk_clear_root_pme_status(port);
	mtk_dev->pme_event = 1;
	spin_unlock_irqrestore(&mtk_dev->lock, flags);

	return IRQ_HANDLED;
}

static bool pci_ltr_supported(struct pci_dev *dev)
{
	u32 cap;

	pci_read_config_dword(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCAP2, &cap);

	return cap & PCI_EXP_DEVCAP2_LTR;
}

static int __pci_ltr_scale(u64 *val)
{
	int scale = 0;

	while (*val > 1023) {
		*val = (*val + 31) / 32;
		scale++;
	}
	return scale;
}

int mtk_pci_set_ltr(struct pci_dev *dev, u64 snoop_lat_ns, u64 nosnoop_lat_ns)
{
	int pos, ret, snoop_scale, nosnoop_scale;
	u16 val;

	if (!pci_ltr_supported(dev))
		return -ENOTSUPP;

	snoop_scale = __pci_ltr_scale(&snoop_lat_ns);
	nosnoop_scale = __pci_ltr_scale(&nosnoop_lat_ns);

	if (snoop_lat_ns > PCI_LTR_VALUE_MASK ||
		nosnoop_lat_ns > PCI_LTR_VALUE_MASK)
		return -EINVAL;

	if ((snoop_scale > (PCI_LTR_SCALE_MASK >> PCI_LTR_SCALE_SHIFT)) ||
		(nosnoop_scale > (PCI_LTR_SCALE_MASK >> PCI_LTR_SCALE_SHIFT)))
		return -EINVAL;

	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_LTR);
	if (!pos)
		return -ENOTSUPP;

	val = (snoop_scale << PCI_LTR_SCALE_SHIFT) | snoop_lat_ns;
	ret = pci_write_config_word(dev, pos + PCI_LTR_MAX_SNOOP_LAT, val);
	if (ret)
		return -EIO;

	val = (nosnoop_scale << PCI_LTR_SCALE_SHIFT) | nosnoop_lat_ns;
	ret = pci_write_config_word(dev, pos + PCI_LTR_MAX_NOSNOOP_LAT, val);
	if (ret)
		return -EIO;

	return 0;
}

static int mtk_pci_enable_ltr(struct pci_dev *dev)
{
	int ret;
	u16 val;

	/* Only primary function can enable/disable LTR */
	if (PCI_FUNC(dev->devfn) != 0)
		return -EINVAL;

	if (!pci_ltr_supported(dev))
		return -ENOTSUPP;

	/* Enable upstream ports first */
	if (dev->bus->self) {
		ret = mtk_pci_enable_ltr(dev->bus->self);
		if (ret)
			return ret;
	}

	ret = pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &val);
	val |= PCI_EXP_LTR_EN;
	ret = pci_write_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, val);

	return ret;
}

void mtk_pci_disable_ltr(struct pci_dev *dev)
{
	u16 val;

	/* Only primary function can enable/disable LTR */
	if (PCI_FUNC(dev->devfn) != 0)
		return;

	if (!pci_ltr_supported(dev))
		return;

	pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &val);
	val &= ~PCI_EXP_LTR_EN;
	pci_write_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, val);
}

u64 get_ltr_ns(unsigned short regValue)
{
	unsigned int scale;
	unsigned int value;

	scale = (regValue >> 10) & 0x7;
	value = regValue & 0x3ff;

	return (u64) ((1 << (scale * 5)) * value);
}

int mtk_pcie_ltr(int enable)
{
	int ret;
	struct pci_dev *dev;
	u16 val;
	unsigned long start;
	unsigned short snoop_latency, no_snoop_latency;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pci_find_capability(dev, PCI_CAP_ID_EXP);

	/* Enable LTR */
	if (enable) {

		pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &val);
		if (val & PCI_EXP_LTR_EN) {
			pr_err("LTR already enabled\n");
			return 0;
		}

		ret = mtk_pci_enable_ltr(dev);
		if (ret) {
			pr_err("[%s:%d], enable LTR fail (%d)\n", __func__, __LINE__, ret);
			return ret;
		}

		/* Rx LTR Message from EP */
		start = jiffies;

		for (;;) {
			if (READ_REGISTER_UINT32(MTK_INT_STATUS) & MTK_INT_LTR_MSG) {

				WRITE_REGISTER_UINT32(MTK_INT_STATUS, READ_REGISTER_UINT32(MTK_INT_STATUS)
									  | MTK_INT_LTR_MSG);
				snoop_latency = READ_REGISTER_UINT32(MTK_REG_LTR_LATENCY) & 0xFFFF;
				no_snoop_latency = (READ_REGISTER_UINT32(MTK_REG_LTR_LATENCY) >> 16) & 0xFFFF;
				break;
			} else if (time_after(jiffies, start + PCIE_DMA_TIMEOUT)) {
				pr_err("[%s:%d], Rx LTR message fail\n", __func__, __LINE__);
				return -1;
			}
		}

		/* Set LTR value */
		ret = mtk_pci_set_ltr(dev, get_ltr_ns(snoop_latency), get_ltr_ns(no_snoop_latency));
		if (ret) {
			pr_err("[%s:%d], set LTR fail\n", __func__, __LINE__);
			return ret;
		}
	} else
		mtk_pci_disable_ltr(dev);


	return 0;
}

int mtk_pci_enable_obff(struct pci_dev *dev, int obff_type)
{
	u32 cap;
	u16 ctrl;
	int ret;

	pci_read_config_dword(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCAP2, &cap);
	if (!(cap & PCI_EXP_OBFF_MASK))
		return -ENOTSUPP; /* no OBFF support at all */

	pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &ctrl);
	ctrl &= ~PCI_EXP_OBFF_WAKE_EN;
	if ((cap & PCI_EXP_OBFF_WAKE) && (obff_type == MTK_OBFF_TYPE_WAKE))
		ctrl |= PCI_EXP_OBFF_WAKE_EN;
	else if ((cap & PCI_EXP_OBFF_MSG) && (obff_type == MTK_OBFF_TYPE_MSGA))
		ctrl |= PCI_EXP_OBFF_MSGA_EN;
	else if ((cap & PCI_EXP_OBFF_MSG) && (obff_type == MTK_OBFF_TYPE_MSGB))
		ctrl |= PCI_EXP_OBFF_MSGB_EN;
	else {
		pr_debug("[%s:%d], OBFF capability support using %s only, but enable type = %s\n",
			   __func__, __LINE__,
			   (cap & PCI_EXP_OBFF_MASK) == PCI_EXP_OBFF_MSG ? "Message" : "Wake#",
			   obff_type == MTK_OBFF_TYPE_WAKE ? "Wake#" : "Message"
			  );
		return -ENOTSUPP;
	}
	pci_write_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, ctrl);
	pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &ctrl);

	/* Make sure the topology supports OBFF as well */
	if (dev->bus->self) {
		ret = mtk_pci_enable_obff(dev->bus->self, obff_type);
		if (ret)
			return ret;
	}

	return 0;
}

void mtk_pci_disable_obff(struct pci_dev *dev)
{
	u16 val;

	pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &val);
	val &= ~PCI_EXP_OBFF_WAKE_EN;
	pci_write_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, val);

	pci_read_config_word(dev, pci_pcie_cap(dev) + PCI_EXP_DEVCTL2, &val);
	pr_debug("[%s:%d], val = 0x%x\n", __func__, __LINE__, val);
	if (dev->bus->self)
		mtk_pci_disable_obff(dev->bus->self);

}

int mtk_rc_drive_clkreq(int enable)
{
	u32 reg32;

	reg32 = READ_REGISTER_UINT32(MTK_REG_WAKE_CONTROL);
	reg32 &= ~mtk_rg_rc_drive_clkreq_dis;

	if (enable)
		reg32 |= mtk_rg_rc_drive_clkreq_dis;

	WRITE_REGISTER_UINT32(MTK_REG_WAKE_CONTROL, reg32);

	return 0;
}

int mtk_l1ss_mode_en(struct pci_dev *dev, int enable)
{
	int pos;
	u32 reg32;

	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_L1PMSS);
	if (!pos) {
		pr_err("L1 PM Substate capability is not found!\n");
		return -1;
	}
	pci_read_config_dword(dev, pos + PCI_L1PMSS_CAP, &reg32);
	if (!(reg32 & PCI_L1PM_CAP_L1PM_SS)) {
		pr_err("not support L1 PM Substates!\n");
		return -1;
	}
	if ((enable & PCI_L1PM_CAP_PCIPM_L12) & (!(reg32 & PCI_L1PM_CAP_PCIPM_L12))) {
		pr_err("not support PCI-PM L1.2!\n");
		return -1;
	}
	if ((enable & PCI_L1PM_CAP_PCIPM_L11) & (!(reg32 & PCI_L1PM_CAP_PCIPM_L11))) {
		pr_err("not support PCI-PM L1.1!\n");
		return -1;
	}
	if ((enable & PCI_L1PM_CAP_ASPM_L12) & (!(reg32 & PCI_L1PM_CAP_ASPM_L12))) {
		pr_err("not support ASPM L1.2!\n");
		return -1;
	}
	if ((enable & PCI_L1PM_CAP_ASPM_L11) & (!(reg32 & PCI_L1PM_CAP_ASPM_L11))) {
		pr_err("not support ASPM L1.1!\n");
		return -1;
	}
	pci_read_config_dword(dev, pos + PCI_L1PMSS_CTR1, &reg32);
	reg32 &= ~PCI_L1PMSS_ENABLE_MASK;
	reg32 |= enable;
	pci_write_config_dword(dev, pos + PCI_L1PMSS_CTR1, reg32);

	return 0;
}

int mtk_setup_clkpm(struct pci_dev *dev, int enable)
{
	int pos;
	u32 reg32;

	pos = dev->pcie_cap;
	pci_read_config_dword(dev, pos + PCI_EXP_LNKCAP, &reg32);
	if (!(reg32 & PCI_EXP_LNKCAP_CLKPM)) {
		pr_err("not support clock PM!\n");
		return -1;
	}

	mtk_rc_drive_clkreq(enable);

	pci_read_config_dword(dev, pos + PCI_EXP_LNKCTL, &reg32);
	reg32 &= ~PCI_EXP_LNKCTL_CLKREQ_EN;
	if (enable)
		reg32 |= PCI_EXP_LNKCTL_CLKREQ_EN;

	pci_write_config_word(dev, pos + PCI_EXP_LNKCTL, reg32);

	return 0;
}


#define PORT_CMR_TIME 0x28

int mtk_setup_l1ss_timing(struct pci_dev *dev)
{
	int pos, ppos, rs_time, prs_time;
	u32 reg32;
	struct pci_dev *pdev;


	pdev = dev->bus->self;
	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_L1PMSS);
	if (!pos) {
		pr_err("L1 PM Substate capability is not found!\n");
		return -1;
	}
	ppos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_L1PMSS);
	if (!ppos) {
		pr_err("L1 PM Substate capability is not found!\n");
		return -1;
	}
	pci_read_config_dword(dev, pos + PCI_L1PMSS_CAP, &reg32);
	rs_time = (reg32 & PCI_L1PM_CAP_PORT_CMR_TIME_MASK) >> 8;
	pci_read_config_dword(pdev, ppos + PCI_L1PMSS_CAP, &reg32);
	prs_time = (reg32 & PCI_L1PM_CAP_PORT_CMR_TIME_MASK) >> 8;
	rs_time = (rs_time > prs_time) ? rs_time : prs_time;
	rs_time = (rs_time > PORT_CMR_TIME) ? rs_time : PORT_CMR_TIME;
	pci_read_config_dword(dev, pos + PCI_L1PMSS_CTR1, &reg32);
	reg32 = (reg32 & PCI_L1PM_CTR1_CMR_TIME_MASK);
	reg32 |= (rs_time << 8);
	pci_write_config_word(dev, pos + PCI_L1PMSS_CTR1, reg32);
	pci_read_config_dword(pdev, ppos + PCI_L1PMSS_CTR1, &reg32);
	reg32 = (reg32 & PCI_L1PM_CTR1_CMR_TIME_MASK);
	reg32 |= (rs_time << 8);
	pci_write_config_word(pdev, ppos + PCI_L1PMSS_CTR1, reg32);

	return 0;
}

int mtk_l1ss_enable(struct pci_dev *dev, int enable)
{
	int ret;

	ret = mtk_l1ss_mode_en(dev, enable);
	if (ret) {
		pr_err("Can't enable L1 PM Substates!\n");
		return -1;
	}

	return 0;
}

int mtk_setup_l1pm(int enable)
{

	struct pci_dev *dev, *pdev;
	int ret;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pdev = dev->bus->self;

	ret = mtk_setup_clkpm(dev, enable);
	if (ret)
		return -1;

	return 0;
}

int mtk_setup_l1ss(int ds_mode, int us_mode)
{

	struct pci_dev *dev, *pdev;
	int ret;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	pdev = dev->bus->self;
	ret = mtk_l1ss_enable(pdev, ds_mode);
	if (ret)
		return -1;

	ret = mtk_l1ss_enable(dev, us_mode);
	if (ret)
		return -1;

	mtk_rc_drive_clkreq(ds_mode);

	return 0;
}


struct tx_desc {
	/* DWORD 0 */
	u32 SDP0;

	/* DWORD 1 */
	u32 SDL1:14;
	u32 LS1:1;
	u32 BURST:1;
	u32 SDL0:14;
	u32 LS0:1;
	u32 DDONE:1;

	/* DWORD 2 */
	u32 SDP1;

	/* DWORD 3 */
	u32 TX_INFO;
};

struct rx_desc {
	/* Word	0 */
	u32 SDP0;

	/* Word	1 */
	u32 SDL1:14;
	u32 LS1:1;
	u32 BURST:1;
	u32 SDL0:14;
	u32 LS0:1;
	u32 DDONE:1;

	/* Word	2 */
	u32 SDP1;

	/* DWORD 3 */
	u32 RX_INFO;
};

int mtk_init_txrx_ring(struct mtk_pci_dev *mtk_dev)
{
	struct tx_desc *tx_desc;
	struct rx_desc *rx_desc;
	int ret = 0, i;

	pr_debug("mtk_init_txrx_ring\n");

	/* Disable TX/RX DMA  --> mt_asic_init_txrx_ring */
	RESET_REG32(MT_WPDMA_GLO_CFG, (BIT(0) + BIT(2)));

	/* Delayed Interrupt Configuration :
	 * WPDMA_DELAY_INT_CFG = user defined  -->  mt_asic_init_txrx_ring
	 */
	WRITE_REG32(MT_DELAY_INT_CFG, 0);

	/* hook tx descriptor to HIF TX Ring Control 0 register. */
	WRITE_REG32(MT_TX_RING_PTR, mtk_dev->tx_desc_phy);
	WRITE_REG32(MT_TX_RING_CNT, TX_RING_SIZE);
	WRITE_REG32(MT_TX_RING_CIDX, 0);
	WRITE_REG32(MT_TX_RING_DIDX, 0);

	tx_desc = (struct tx_desc *)mtk_dev->tx_desc_buffer;

	/* chain the tx ring buffer. */
	for (i = 0; i < TX_RING_SIZE; i++) {

		/* base address */
		tx_desc->SDP0 = mtk_dev->tx_ring_phy + MAX_DMA_LENGTH * i;
		/* transfer length */
		tx_desc->SDL0 = 0;
		tx_desc->DDONE = 0;

		tx_desc++;
	}

	/* hook rx descriptor to HIF RX Ring Control 0 register. */
	WRITE_REG32(MT_RX_RING_PTR, mtk_dev->rx_desc_phy);
	WRITE_REG32(MT_RX_RING_CNT, RX_RING_SIZE);
	WRITE_REG32(MT_RX_RING_CIDX, 4);
	WRITE_REG32(MT_RX_RING_DIDX, 0);

	rx_desc = (struct rx_desc *)mtk_dev->rx_desc_buffer;

	/* chain the rx ring buffer. */
	for (i = 0; i < RX_RING_SIZE; i++) {

		/* base address */
		rx_desc->SDP0 = mtk_dev->rx_ring_phy + MAX_DMA_LENGTH * i;
		/* transfer length */
		rx_desc->SDL0 = 0;
		rx_desc->DDONE = 0;

		rx_desc++;
	}

	/* reset CPU idx and DMA idx */
	WRITE_REG32(WPDMA_RST_PTR, 0xffffffff);

	/*
	 * enable tx/rx done interrupt.
	 */
	SET_REG32(MT_INT_MASK_CSR, MT_INT_T0_DONE + MT_INT_R0_DONE);

	/*
	 * clear interrupt status.
	 */
	WRITE_REG32(MT_INT_SOURCE_CSR, 0xffffffff);

	/* Init DMA  -->  mt_hif_sys_init */
	WRITE_REG32(MT_WPDMA_GLO_CFG, 0x10001870);

	/* enable tx/rx DMA */
	SET_REG32(MT_WPDMA_GLO_CFG, (BIT(0) + BIT(2) + BIT(4) + BIT(5) + BIT(6)));

	return ret;
}

int mtk_create_txrx_ring(void)
{
	int ret = 0;
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;

	pr_debug("mtk_create_txrx_ring\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);

	/* alloc tx/rx descriptor space. */
	mtk_dev->tx_desc_buffer = dma_alloc_coherent(&dev->dev,
						TXD_SIZE * TX_RING_SIZE,
						&mtk_dev->tx_desc_phy,
						GFP_KERNEL | GFP_DMA);

	mtk_dev->rx_desc_buffer = dma_alloc_coherent(&dev->dev,
						RXD_SIZE * RX_RING_SIZE,
						&mtk_dev->rx_desc_phy,
						GFP_KERNEL | GFP_DMA);

	/* alloc tx/rx ring buffer. */
	mtk_dev->tx_ring_buffer = dma_alloc_coherent(&dev->dev,
						MAX_DMA_LENGTH * TX_RING_SIZE,
						&mtk_dev->tx_ring_phy,
						GFP_KERNEL | GFP_DMA);

	mtk_dev->rx_ring_buffer = dma_alloc_coherent(&dev->dev,
						MAX_DMA_LENGTH * RX_RING_SIZE,
						&mtk_dev->rx_ring_phy,
						GFP_KERNEL | GFP_DMA);

	/* init descriptor and chain to a ring. */
	mtk_init_txrx_ring(mtk_dev);

	return ret;
}


int mtk_free_txrx_ring(void)
{
	int ret = 0;
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;

	pr_debug("mtk_alloc_txrx_ring\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);

	dma_free_coherent(NULL, TXD_SIZE * TX_RING_SIZE,
			mtk_dev->tx_desc_buffer, mtk_dev->tx_desc_phy);

	dma_free_coherent(NULL, RXD_SIZE * RX_RING_SIZE,
			mtk_dev->rx_desc_buffer, mtk_dev->rx_desc_phy);

	dma_free_coherent(NULL, MAX_DMA_LENGTH * TX_RING_SIZE,
			mtk_dev->tx_ring_buffer, mtk_dev->tx_ring_phy);

	dma_free_coherent(NULL, MAX_DMA_LENGTH * RX_RING_SIZE,
			mtk_dev->rx_ring_buffer, mtk_dev->rx_ring_phy);

	return ret;
}

struct pkt_header {
	u32 signature;
	u16 pkt_len;
	u8 dma_mode;
	u8 pattern;
};

struct tx_packet {
	struct pkt_header header;
	u8 *payload;
};

struct mtk_pci_dev *g_mtk_dev;

int mtk_gen_tx_packet(u32 pkt_len)
{
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;

	u32 dma_idx;
	u8 *tx_ring;
	int ret = 0;

	pr_debug("mtk_gen_tx_packet\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);
	g_mtk_dev = mtk_dev;

	dma_idx = READ_REG32(MT_TX_RING_DIDX);
	tx_ring = mtk_dev->tx_ring_buffer + MAX_DMA_LENGTH * dma_idx;

	if (pkt_len >= 4096)
		return -1;

	get_random_bytes(tx_ring, pkt_len);

	return ret;
}

int mtk_wait_dma_tx_done(struct mtk_pci_dev *mtk_dev)
{
	u32 cpu_idx, dma_idx;
	int ret = 0;

	cpu_idx = READ_REG32(MT_TX_RING_CIDX);
	dma_idx = READ_REG32(MT_TX_RING_DIDX);

	if (!wait_for_completion_timeout(&mtk_dev->tx_done, PCIE_TX_done_TIMEOUT)) {
		pr_err("mtk_wait_dma_tx_done timeout!!!\n");
		ret = -1;
	}

	return ret;
}

int mtk_wait_dma_rx_done(struct mtk_pci_dev *mtk_dev)
{
	int ret = 0;

	if (!wait_for_completion_timeout(&mtk_dev->rx_done, PCIE_RX_done_TIMEOUT)) {
		pr_err("mtk_wait_dma_rx_done timeout!!!\n");
		ret = -1;
	}

	return ret;
}

int mtk_transmit_tx_packet(u32 pkt_len)
{
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;
	struct tx_desc *tx_desc;
	struct rx_desc *rx_desc;
	u8 *tx_ring, *rx_ring;
	u32 cpu_idx, old_cpuidx, dma_idx;
	int ret = 0;

	pr_debug("mtk_transmit_tx_packet\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);

	init_completion(&mtk_dev->tx_done);
	init_completion(&mtk_dev->rx_done);

	old_cpuidx = cpu_idx = READ_REG32(MT_TX_RING_CIDX);
	dma_idx = READ_REG32(MT_TX_RING_DIDX);

	/* update tx descriptor info. */
	tx_desc = (struct tx_desc *)mtk_dev->tx_desc_buffer + cpu_idx;
	tx_desc->SDL0 = pkt_len;
	tx_desc->LS0 = 1;
	tx_desc->LS1 = 0;
	tx_desc->BURST = 0;
	tx_desc->DDONE = 0;
	tx_desc->SDL1 = 0;
	tx_desc->SDP1 = 0;
	mb(); /* For memory barrier */

	tx_ring = mtk_dev->tx_ring_buffer + MAX_DMA_LENGTH * cpu_idx;

	dma_idx = READ_REG32(MT_RX_RING_DIDX);

	/* update rx descriptor info. */
	rx_desc = (struct rx_desc *)mtk_dev->rx_desc_buffer + dma_idx;
	rx_desc->SDL0 = pkt_len;
	rx_desc->DDONE = 0;

	rx_ring = mtk_dev->rx_ring_buffer + MAX_DMA_LENGTH * dma_idx;

	/* trigger rx */
	cpu_idx = READ_REG32(MT_RX_RING_CIDX);
	cpu_idx = ((cpu_idx + 1) % RX_RING_SIZE);
	WRITE_REG32(MT_RX_RING_CIDX, cpu_idx);

	/* trigger tx */
	cpu_idx = READ_REG32(MT_TX_RING_CIDX);
	cpu_idx = ((cpu_idx + 1) % TX_RING_SIZE);
	WRITE_REG32(MT_TX_RING_CIDX, cpu_idx);

	ret = mtk_wait_dma_tx_done(mtk_dev);

	return ret;
}

int mtk_receive_rx_packet(u32 pkt_len)
{
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;
	struct rx_desc *rx_desc;
	u32 cpu_idx, dma_idx;
	int ret = 0;

	pr_debug("mtk_receive_rx_packet\n");

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);


	ret = mtk_wait_dma_rx_done(mtk_dev);
	if (ret)
		return -1;

	cpu_idx = READ_REG32(MT_TX_RING_CIDX);
	dma_idx = READ_REG32(MT_TX_RING_DIDX);
	dma_idx =  dma_idx ? (dma_idx - 1) : (RX_RING_SIZE - 1);

	rx_desc = (struct rx_desc *)mtk_dev->rx_desc_buffer + dma_idx;
	rx_desc->DDONE = 0;

	return ret;
}

int mtk_compare_txrx_packet(u32 pkt_len)
{
	struct pci_dev *dev;
	struct mtk_pci_dev *mtk_dev;
	u32 dma_idx, i, didvid;
	u8 *tx_buffer, *rx_buffer;
	int ret = 0, err_cnt = 0;

	dev = mtk_get_pcie_resource(PCI_CLASS_MTK_DEVICE);
	if (!dev)
		return -1;

	mtk_dev = pci_get_drvdata(dev);

	dma_idx = READ_REG32(MT_TX_RING_DIDX);

	if (dma_idx)
		dma_idx -= 1;
	else
		dma_idx = RX_RING_SIZE - 1;

	tx_buffer = mtk_dev->tx_ring_buffer + MAX_DMA_LENGTH * dma_idx;
	rx_buffer = mtk_dev->rx_ring_buffer + MAX_DMA_LENGTH * dma_idx;

	for (i = 0; i < pkt_len; i++) {
		if (tx_buffer[i] != rx_buffer[i]) {
			pr_err("mtk_compare_txrx_packet error!!!!tx[%d]=%x,rx[%d]=%x\n",
				i, tx_buffer[i], i, rx_buffer[i]);
			err_cnt++;
		} else
			err_cnt = 0;

		if (err_cnt == 2) {
			pr_err("compare error!!! %x, %x\n",
				*((u32 *)tx_buffer + i), *((u32 *)rx_buffer + i));
			/* for LA trigger. */
			pci_bus_read_config_dword(dev->bus, dev->devfn, 0x00, &didvid);
			ret = -1;
			break;
		}
	}

	return ret;
}
