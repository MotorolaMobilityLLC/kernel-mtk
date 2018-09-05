/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/mman.h>
#include <linux/dmapool.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <asm/types.h>
#include <linux/wait.h>
#include <linux/timer.h>

#define DRIVER_NAME "mtk_mdla"

#include <linux/interrupt.h>
#include "gsm.h"
#include "mdla_pmu.h"
#include <linux/spinlock.h>

#include "mdla_hw_reg.h"
#include "mdla_ioctl.h"
#include "mdla_ion.h"
#include "mdla_debug.h"

#define  DEVICE_NAME "mdlactl"
#define  CLASS_NAME  "mdla"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen, Wen");
MODULE_DESCRIPTION("MDLA driver");
MODULE_VERSION("0.1");

static void *apu_mdla_cmde_mreg_top;
static void *apu_mdla_config_top;
void *apu_mdla_biu_top;
void *apu_mdla_gsm_top;

int mdla_irq;

/* TODO: move these global control vaiables into device specific data.
 * to support multiple instance (multiple MDLA).
 */
static int majorNumber;
static int numberOpens;
static struct class *mdlactlClass;
static struct device *mdlactlDevice;
static struct platform_device *mdlactlPlatformDevice;
static u32 cmd_id;
static u32 max_cmd_id;
static u32 cmd_list_len;
struct work_struct mdla_queue;

#define UINT32_MAX (0xFFFFFFFF)
static u32 last_cmd_id0 = UINT32_MAX;
static u32 last_cmd_id1 = UINT32_MAX;
static u32 fail_cmd_id0 = UINT32_MAX;
static u32 fail_cmd_id1 = UINT32_MAX;

DEFINE_MUTEX(cmd_list_lock);
static DECLARE_WAIT_QUEUE_HEAD(mdla_cmd_queue);
static LIST_HEAD(cmd_list);

struct mtk_mdla_local {
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};
struct command_entry {
	struct list_head list;
	void *kva;    /* Virtual Address for Kernel */
	u32 mva;      /* Physical Address for Device */
	u32 count;
	u32 id;
	u64 khandle;  /* ion kernel handle */
	u8 type;      /* allocate memory type */
};

static int mdla_open(struct inode *, struct file *);
static int mdla_release(struct inode *, struct file *);
static long mdla_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int mdla_wait_command(u32 id);
static int mdla_mmap(struct file *filp, struct vm_area_struct *vma);
static int mdla_process_command(struct command_entry *run_cmd_data);
static void mdla_timeup(unsigned long data);

#define MDLA_TIMEOUT_DEFAULT 1000 /* ms */
u32 mdla_timeout = MDLA_TIMEOUT_DEFAULT;
static DEFINE_TIMER(mdla_timer, mdla_timeup, 0, 0);

static const struct file_operations fops = {
	.open = mdla_open,
	.unlocked_ioctl = mdla_ioctl,
	.mmap = mdla_mmap,
	.release = mdla_release,
};

unsigned int mdla_cfg_read(u32 offset)
{
	return ioread32(apu_mdla_config_top + offset);
}

static void mdla_cfg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_config_top + offset);
}

unsigned int mdla_reg_read(u32 offset)
{
	return ioread32(apu_mdla_cmde_mreg_top + offset);
}

static void mdla_reg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_cmde_mreg_top + offset);
}

static void mdla_reset(void)
{
	mdla_debug("%s: MDLA RESET\n", __func__);  // TODO: remove debug
	mdla_cfg_write(0x03f, MDLA_SW_RST);
	mdla_cfg_write(0x000, MDLA_SW_RST);
	mdla_cfg_write(0xffffffff, MDLA_CG_CLR);
	mdla_reg_write(MDLA_IRQ_MASK & ~(MDLA_IRQ_SWCMD_DONE),
		MREG_TOP_G_INTP2);
}

static void mdla_timeup(unsigned long data)
{

	// TODO: remove debug
	mdla_debug("%s: MDLA Timeout: %u ms, max_cmd_id: %u, cmd_id0: %u, cmd_id1: %u\n",
		__func__, mdla_timeout, max_cmd_id, last_cmd_id0, last_cmd_id1);
	mdla_dump_reg();
	mdla_reset();

	fail_cmd_id0 = last_cmd_id0;
	fail_cmd_id1 = last_cmd_id1;

	if (last_cmd_id1 == UINT32_MAX)
		max_cmd_id = last_cmd_id0;
	else
		max_cmd_id = last_cmd_id1;

	/* wake up those who were waiting on id0 or id1. */
	wake_up_interruptible_all(&mdla_cmd_queue);
	/* resume execution of the command queue */
	schedule_work(&mdla_queue);
}

static int mdla_mmap(struct file *filp, struct vm_area_struct *vma)
{

	unsigned long offset = vma->vm_pgoff;
	unsigned long size = vma->vm_end - vma->vm_start;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, offset, size,
			vma->vm_page_prot)) {
		pr_info("%s: remap_pfn_range error: %p\n",
			__func__, vma);
		return -EAGAIN;
	}
	return 0;
}

static void mdla_start_queue(struct work_struct *work)
{
	struct command_entry *run_cmd_data;

	mutex_lock(&cmd_list_lock);

	while (max_cmd_id >= last_cmd_id0) {
		cmd_list_len--;
		last_cmd_id0 = last_cmd_id1;
		last_cmd_id1 = UINT32_MAX;
		if (!list_empty(&cmd_list)) {
			run_cmd_data = list_first_entry(&cmd_list,
					struct command_entry, list);
			list_del(&run_cmd_data->list);
			last_cmd_id1 = run_cmd_data->id + run_cmd_data->count
					- 1;
			mdla_process_command(run_cmd_data);
			kfree(run_cmd_data);
		}
	}
	if ((cmd_list_len == 0) && timer_pending(&mdla_timer)) {
		// TODO: remove debug
		mdla_debug("%s: del_timer().\n", __func__);
		del_timer(&mdla_timer);
	}
	mutex_unlock(&cmd_list_lock);
	wake_up_interruptible_all(&mdla_cmd_queue);
	mdla_debug(
			"mdla_interrupt max_cmd_id: [%d] last_cmd_id0: [%d] last_cmd_id1: [%d]\n",
			max_cmd_id, last_cmd_id0, last_cmd_id1);
}

static irqreturn_t mdla_interrupt(int irq, void *dev_id)
{
	max_cmd_id = mdla_reg_read(MREG_TOP_G_FIN0);
	mdla_reg_write(MDLA_IRQ_MASK, MREG_TOP_G_INTP0);
	schedule_work(&mdla_queue);
	return IRQ_HANDLED;
}

static int mdla_probe(struct platform_device *pdev)
{
	struct resource *r_irq; /* Interrupt resources */
	struct resource *apu_mdla_command; /* IO mem resources */
	struct resource *apu_mdla_config; /* IO mem resources */
	struct resource *apu_mdla_biu; /* IO mem resources */
	struct resource *apu_mdla_gsm; /* IO mem resources */
	int rc = 0;
	struct device *dev = &pdev->dev;

	mdlactlPlatformDevice = pdev;

	dev_info(dev, "Device Tree Probing\n");

	/* Get iospace for MDLA Config */
	apu_mdla_config = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!apu_mdla_config) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}

	/* Get iospace for MDLA Command */
	apu_mdla_command = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!apu_mdla_command) {
		dev_err(dev, "invalid address\n");
		return -ENODEV;
	}
	/* Get iospace for MDAL PMU */
	apu_mdla_biu = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!apu_mdla_command) {
		dev_err(dev, "apu_mdla_biu address\n");
		return -ENODEV;
	}

	/* Get iospace GSM */
	apu_mdla_gsm = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (!apu_mdla_gsm) {
		dev_err(dev, "apu_mdla_biu address\n");
		return -ENODEV;
	}

	apu_mdla_config_top = ioremap_nocache(apu_mdla_config->start,
			apu_mdla_config->end - apu_mdla_config->start + 1);
	if (!apu_mdla_config_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	apu_mdla_cmde_mreg_top = ioremap_nocache(apu_mdla_command->start,
			apu_mdla_command->end - apu_mdla_command->start + 1);
	if (!apu_mdla_cmde_mreg_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	apu_mdla_biu_top = ioremap_nocache(apu_mdla_biu->start,
			apu_mdla_biu->end - apu_mdla_biu->start + 1);
	if (!apu_mdla_biu_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	apu_mdla_gsm_top = ioremap_nocache(apu_mdla_gsm->start,
			apu_mdla_gsm->end - apu_mdla_gsm->start + 1);
	if (!apu_mdla_gsm_top) {
		dev_err(dev, "mtk_mdla: Could not allocate iomem\n");
		rc = -EIO;
		return rc;
	}

	/* Get IRQ for the device */
	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r_irq) {
		dev_info(dev, "no IRQ found\n");
		return rc;
	}
	dev_info(dev, "platform_get_resource irq: %d\n", (int)r_irq->start);

	mdla_irq = r_irq->start;
	rc = request_irq(mdla_irq, mdla_interrupt, IRQF_TRIGGER_LOW,
	DRIVER_NAME, dev);
	dev_info(dev, "request_irq\n");
	if (rc) {
		rc = request_irq(mdla_irq, mdla_interrupt, IRQF_TRIGGER_HIGH,
				DRIVER_NAME, dev);
		if (rc) {
			dev_err(dev, "mtk_mdla: Could not allocate interrupt %d.\n",
					mdla_irq);
			return rc;
		}
	}
	dev_info(dev, "apu_mdla_config_top at 0x%08lx mapped to 0x%08lx\n",
			(unsigned long __force)apu_mdla_config->start,
			(unsigned long __force)apu_mdla_config->end);
	dev_info(dev, "apu_mdla_command at 0x%08lx mapped to 0x%08lx, irq=%d\n",
			(unsigned long __force)apu_mdla_command->start,
			(unsigned long __force)apu_mdla_command->end,
			(int)r_irq->start);
	dev_info(dev, "apu_mdla_biu_top at 0x%08lx mapped to 0x%08lx\n",
			(unsigned long __force)apu_mdla_biu->start,
			(unsigned long __force)apu_mdla_biu->end);

	mdla_ion_init();

	return 0;

}

static int mdla_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	free_irq(mdla_irq, dev);
	iounmap(apu_mdla_config_top);
	iounmap(apu_mdla_cmde_mreg_top);
	iounmap(apu_mdla_biu_top);
	iounmap(apu_mdla_gsm_top);
	mdla_ion_exit();
	platform_set_drvdata(pdev, NULL);

	return 0;
}
static const struct of_device_id mdla_of_match[] = {
	{ .compatible = "mediatek,mdla", },
	{ .compatible = "mtk,mdla", },
	{ /* end of list */},
};

MODULE_DEVICE_TABLE(of, mdla_of_match);
static struct platform_driver mdla_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = mdla_of_match,
	},
	.probe = mdla_probe,
	.remove = mdla_remove,
};

static int mdlactl_init(void)
{
	int ret;

	ret = platform_driver_register(&mdla_driver);
	if (ret != 0)
		return ret;

	numberOpens = 0;
	cmd_id = 1;
	max_cmd_id = 1;
	cmd_list_len = 0;

	// Try to dynamically allocate a major number for the device
	//  more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	/* TODO: replace with alloc_chrdev_region() and
	 * unregister_chrdev_region()
	 * see examples at drivers/misc/mediatek/vpu/1.0/ vpu_reg_chardev,
	 * vpu_unreg_chardev
	 */

	if (majorNumber < 0) {
		pr_warn("MDLA failed to register a major number\n");
		return majorNumber;
	}
	mdla_debug("MDLA: registered correctly with major number %d\n",
			majorNumber);

	// Register the device class
	mdlactlClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(mdlactlClass)) {  // Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		pr_warn("Failed to register device class\n");
		return PTR_ERR(mdlactlClass);
	}
	// Register the device driver
	mdlactlDevice = device_create(mdlactlClass, NULL, MKDEV(majorNumber, 0),
	NULL, DEVICE_NAME);
	if (IS_ERR(mdlactlDevice)) {  // Clean up if there is an error
		unregister_chrdev(majorNumber, DEVICE_NAME);
		pr_warn("Failed to create the device\n");
		return PTR_ERR(mdlactlDevice);
	}

	// Init DMA from of
	of_dma_configure(mdlactlDevice, NULL);

	// Set DMA mask
	if (dma_get_mask(mdlactlDevice) != DMA_BIT_MASK(32)) {
		ret = dma_set_mask_and_coherent(mdlactlDevice,
					DMA_BIT_MASK(32));
		if (ret)
			pr_warn("MDLA: set DMA mask failed: %d\n", ret);
	}

	mdla_reset();
	mdla_dump_reg();

	pmu_init();
	INIT_WORK(&mdla_queue, mdla_start_queue);

	return 0;
}

static int mdla_open(struct inode *inodep, struct file *filep)
{
	numberOpens++;
	mdla_debug("MDLA: Device has been opened %d time(s)\n", numberOpens);

	return 0;
}

static int mdla_release(struct inode *inodep, struct file *filep)
{
	mdla_debug("MDLA: Device successfully closed\n");

	return 0;
}

static int mdla_wait_command(u32 id)
{
	mdla_debug("%s: enter id:[%d]\n", __func__, id);
	// @suppress("Type cannot be resolved")
	wait_event_interruptible(mdla_cmd_queue, max_cmd_id >= id);
	if ((id == fail_cmd_id0) ||	(id == fail_cmd_id1)) {
		mdla_debug("mdla_wait_command exit <timeout> [%d]\n", id);
		return -EIO;
	}

	mdla_debug("%s; exit\n", __func__);

	return 0;
}

static void mdla_mark_command_id(struct command_entry *run_cmd_data)
{
	int i;
	u32 count = run_cmd_data->count;
	u32 *cmd = run_cmd_data->kva;

	run_cmd_data->id = cmd_id;

	/* Patch Command buffer 0x150
	 * CODA: 0x150,	mreg_cmd_swcmd_id, 32, 31, 0, mreg_cmd_swcmd_id,
	 * RW, PRIVATE, 32'h0,, This SW command ID
	 */

	for (i = 0; i < count; i++) {
		mdla_debug("%s: add command: %d\n", __func__, cmd_id);
		cmd[84 + (i * 96)] = cmd_id++;
	}

	// TODO: remove debug logs
	mdla_debug("%s: kva=%p, mva=0x%08x, phys_to_dma=%p\n",
			__func__,
			run_cmd_data->kva,
			run_cmd_data->mva,
			phys_to_dma(mdlactlDevice, run_cmd_data->mva));

#ifdef CONFIG_MTK_MDLA_ION
	if (run_cmd_data->khandle) {
		mdla_ion_sync(run_cmd_data->khandle);
		return;
	}
#endif
	dma_sync_single_for_device(mdlactlDevice,
			phys_to_dma(mdlactlDevice, run_cmd_data->mva),
			count * 0x180, DMA_TO_DEVICE);
}

static int mdla_process_command(struct command_entry *run_cmd_data)
{
	dma_addr_t addr = run_cmd_data->mva;
	u32 count = run_cmd_data->count;
	u32 *cmd = (u32 *) run_cmd_data->kva;

	mdla_debug("%s: id: [%d], count: [%d]\n", cmd[84],
		__func__, run_cmd_data->count);

	if (mdla_timeout) {
		// TODO: remove debug
		mdla_debug("%s: mod_timer(), cmd id=%u, [%u]\n",
			__func__, run_cmd_data->id, cmd[84]);
		mod_timer(&mdla_timer,
			jiffies + msecs_to_jiffies(mdla_timeout));
	}

	/* Issue command */
	mdla_reg_write(addr, MREG_TOP_G_CDMA1);
	mdla_reg_write(count, MREG_TOP_G_CDMA2);
	mdla_reg_write(run_cmd_data->id, MREG_TOP_G_CDMA3);

	return 0;
}

static int mdla_run_command_async(struct ioctl_run_cmd *run_cmd_data)
{
	u32 id;
	struct command_entry *cmd = kmalloc(sizeof(struct command_entry),
	GFP_KERNEL);

	cmd->mva = run_cmd_data->mva;

	if (is_gsm_addr(cmd->mva))
		cmd->kva = gsm_mva_to_virt(cmd->mva);
	else if (run_cmd_data->khandle) {
		cmd->kva = (void *)run_cmd_data->kva;
		mdla_debug("%s: <ion> kva=%p, mva=%08x\n",
				__func__,
				cmd->kva,
				cmd->mva);
	} else {
		cmd->kva = phys_to_virt(cmd->mva);
		mdla_debug("%s: <dram> kva: phys_to_virt(mva:%08x) = %p\n",
			__func__, cmd->mva, cmd->kva);
	}

	cmd->count = run_cmd_data->count;
	cmd->khandle = run_cmd_data->khandle;
	cmd->type = run_cmd_data->type;

	mutex_lock(&cmd_list_lock);

	/* Increate cmd_id*/
	mdla_mark_command_id(cmd);

	id = cmd_id - 1;
	if (cmd_list_len == 0) {
		mdla_debug("cmd_list_len == 0\n");
		last_cmd_id0 = id;
		last_cmd_id1 = UINT32_MAX;
		mdla_process_command(cmd);
		kfree(cmd);
	} else if (cmd_list_len == 1) {
		mdla_debug("cmd_list_len == 1\n");
		last_cmd_id1 = id;
		mdla_process_command(cmd);
		kfree(cmd);
	} else {
		mdla_debug("cmd_list_len == %d\n", cmd_list_len);
		list_add_tail(&cmd->list, &cmd_list);
	}
	cmd_list_len++;
	mutex_unlock(&cmd_list_lock);

	return id;
}

static void mdla_dram_alloc(struct ioctl_malloc *malloc_data)
{
	dma_addr_t phyaddr = 0;

	malloc_data->kva = dma_alloc_coherent(mdlactlDevice, malloc_data->size,
			&phyaddr, GFP_KERNEL);
	malloc_data->mva = dma_to_phys(mdlactlDevice, phyaddr);
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
}

static void mdla_dram_free(struct ioctl_malloc *malloc_data)
{
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
	dma_free_coherent(mdlactlDevice, malloc_data->size,
			(void *) malloc_data->kva, malloc_data->mva);
}

static void mdla_gsm_alloc(struct ioctl_malloc *malloc_data)
{
	malloc_data->kva = gsm_alloc(malloc_data->size);
	malloc_data->mva = gsm_virt_to_mva(malloc_data->kva);
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
}
static void mdla_gsm_free(struct ioctl_malloc *malloc_data)
{
	if (malloc_data->kva)
		gsm_release(malloc_data->kva, malloc_data->size);
	mdla_debug("%s: kva:%p, mva:%x\n", // TODO: remove debug
		__func__, malloc_data->kva, malloc_data->mva);
}

static long mdla_ioctl(struct file *filp, unsigned int command,
		unsigned long arg)
{
	long retval;
	struct ioctl_malloc malloc_data;
	struct ioctl_run_cmd run_cmd_data;
	struct ioctl_perf perf_data;
	u32 id;

	switch (command) {
	case IOCTL_MALLOC:

		if (copy_from_user(&malloc_data, (void *) arg,
			sizeof(malloc_data))) {
			retval = -EFAULT;
			return retval;
		}

		if (malloc_data.type == MEM_DRAM)
			mdla_dram_alloc(&malloc_data);
		else if (malloc_data.type == MEM_GSM)
			mdla_gsm_alloc(&malloc_data);

		if (copy_to_user((void *) arg, &malloc_data,
			sizeof(malloc_data)))
			return -EFAULT;
		mdla_debug("ioctl_malloc size:[%x] pva:[%x] kva:[%p]\n",
				malloc_data.size,
				malloc_data.mva,
				malloc_data.kva);
		if (malloc_data.kva == NULL)
			return -ENOMEM;
		break;
	case IOCTL_FREE:

		if (copy_from_user(&malloc_data, (void *) arg,
				sizeof(malloc_data))) {
			retval = -EFAULT;
			return retval;
		}

		if (malloc_data.type == MEM_DRAM)
			mdla_dram_free(&malloc_data);
		else if (malloc_data.type == MEM_GSM)
			mdla_gsm_free(&malloc_data);
		mdla_debug("ioctl_free size:[%x] pa:[%x] pva:[%p]\n",
				malloc_data.size, malloc_data.mva,
				malloc_data.kva);
		break;
	case IOCTL_RUN_CMD_SYNC:
		if (copy_from_user(&run_cmd_data, (void *) arg,
				sizeof(run_cmd_data))) {
			retval = -EFAULT;
			return retval;
		}

		mdla_debug("%s: RUN_CMD_SYNC: kva=%p, mva=0x%08x, phys_to_virt=%p\n",
				__func__,
				(void *)run_cmd_data.kva,
				run_cmd_data.mva,
				phys_to_virt(run_cmd_data.mva));
		id = mdla_run_command_async(&run_cmd_data);
		retval = mdla_wait_command(id);
		return retval;

	case IOCTL_RUN_CMD_ASYNC:
		if (copy_from_user(&run_cmd_data, (void *) arg,
				sizeof(run_cmd_data))) {
			retval = -EFAULT;
			return retval;
		}

		mdla_debug("%s: RUN_CMD_ASYNC: kva=%p, mva=0x%08x, phys_to_virt=%p\n",
				__func__,
				(void *)run_cmd_data.kva,
				run_cmd_data.mva,
				phys_to_virt(run_cmd_data.mva));
		id = mdla_run_command_async(&run_cmd_data);
		run_cmd_data.id = id;
		if (copy_to_user((void *) arg, &run_cmd_data,
			sizeof(run_cmd_data)))
			return -EFAULT;
		break;
	case IOCTL_WAIT_CMD:
		if (copy_from_user(&run_cmd_data, (void *) arg,
				sizeof(run_cmd_data))) {
			retval = -EFAULT;
			return retval;
		}
		retval = mdla_wait_command(run_cmd_data.id);
		return retval;

	case IOCTL_PERF_SET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.handle = pmu_set_perf_event(
			perf_data.interface, perf_data.event);
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.event = pmu_get_perf_event(perf_data.handle);
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_CNT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.counter = pmu_get_perf_counter(perf_data.handle);

		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;

	case IOCTL_PERF_UNSET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		pmu_unset_perf_event(perf_data.handle);
		break;
	case IOCTL_PERF_GET_START:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.start = pmu_get_perf_start();
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_END:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.end = pmu_get_perf_end();
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_CYCLE:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.start = pmu_get_perf_cycle();
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_RESET_CNT:
		pmu_reset_counter();
		break;
	case IOCTL_PERF_RESET_CYCLE:
		pmu_reset_cycle();
		break;
	case IOCTL_PERF_SET_MODE:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		pmu_perf_set_mode(perf_data.mode);
		break;
	case IOCTL_ION_KMAP:
		return mdla_ion_kmap(arg);
	case IOCTL_ION_KUNMAP:
		return mdla_ion_kunmap(arg);
	default:
		return -EINVAL;
	}
	return 0;
}

static void mdlactl_exit(void)
{
	platform_driver_unregister(&mdla_driver);
	device_destroy(mdlactlClass, MKDEV(majorNumber, 0));
	class_destroy(mdlactlClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	mdla_debug("MDLA: Goodbye from the LKM!\n");
}

module_init(mdlactl_init);
module_exit(mdlactl_exit);
