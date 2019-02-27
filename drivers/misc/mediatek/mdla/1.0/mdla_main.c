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

#include "mdla_debug.h"
#include "mdla.h"
#include "gsm.h"
#include "mdla_pmu.h"
#include "mdla_hw_reg.h"
#include "mdla_ioctl.h"
#include "mdla_ion.h"
#include "mdla_trace.h"
#include "mdla_dvfs.h"

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
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

// #define MTK_MDLA_ALWAYS_POWER_ON

#ifdef MTK_MDLA_ALWAYS_POWER_ON
u32 dbg_power_on;
#endif

#define DRIVER_NAME "mtk_mdla"
#define DEVICE_NAME "mdlactl"
#define CLASS_NAME  "mdla"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen, Wen");
MODULE_DESCRIPTION("MDLA driver");
MODULE_VERSION("0.1");

static void *apu_mdla_cmde_mreg_top;
static void *apu_mdla_config_top;
void *apu_mdla_biu_top;
void *apu_mdla_gsm_top;
void *apu_mdla_gsm_base;

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

DEFINE_MUTEX(cmd_list_lock);
static DECLARE_WAIT_QUEUE_HEAD(mdla_cmd_queue);
static LIST_HEAD(cmd_list);
static LIST_HEAD(cmd_fin_list);

u32 mdla_max_cmd_id(void)
{
	return max_cmd_id;
}

struct mtk_mdla_local {
	int irq;
	unsigned long mem_start;
	unsigned long mem_end;
	void __iomem *base_addr;
};

static inline void command_entry_end(struct command_entry *ce)
{
	if (ce && ce->id <= max_cmd_id)
		ce->req_end_t = sched_clock();
}

static inline void command_entry_result(
	struct command_entry *ce, int result)
{
	if (ce)
		ce->result = result;
}

/* MDLA internal FIFO, protected by cmd_list_lock */
#define MDLA_FIFO_SIZE 2

#if (MDLA_FIFO_SIZE == 1)
static u32 last_cmd_id = UINT32_MAX;
static u32 fail_cmd_id = UINT32_MAX;
static struct command_entry *last_cmd_ent;
static inline
struct command_entry *mdla_fifo_head(void)
{
	return last_cmd_ent;
}

static inline u32 mdla_fifo_head_id(void)
{
	return last_cmd_id;
}

static inline u32 mdla_fifo_tail_id(void)
{
	return last_cmd_id;
}

static inline bool mdla_fifo_is_empty(void)
{
	return (cmd_list_len == 0);
}

static int mdla_fifo_in(u32 id, struct command_entry *ce)
{
	if (cmd_list_len == 0) {
		mdla_debug("cmd_list_len == 0\n");
		last_cmd_id = id;
		last_cmd_ent = ce;
		cmd_list_len++;
		return 1;
	}

	mdla_debug("cmd_list_len == %d\n", cmd_list_len);
	return 0;
}

static void mdla_fifo_out(void)
{
	if (!cmd_list_len)
		mdla_debug("%s: MDLA FIFO is already empty\n", __func__);

	cmd_list_len--;
	last_cmd_id = UINT32_MAX;
	last_cmd_ent = NULL;
}

static inline void mdla_fifo_mark_fail(void)
{
	command_entry_result(last_cmd_ent, MDLA_CMD_TIMEOUT);
}

static inline void mdla_fifo_mark_time(void)
{
	command_entry_end(last_cmd_ent);
}

#elif (MDLA_FIFO_SIZE == 2)
static u32 last_cmd_id0 = UINT32_MAX;
static u32 last_cmd_id1 = UINT32_MAX;
static struct command_entry *last_cmd_ent0;
static struct command_entry *last_cmd_ent1;
static inline
struct command_entry *mdla_fifo_head(void)
{
	return last_cmd_ent0;
}
static inline u32 mdla_fifo_head_id(void)
{
	return last_cmd_id0;
}

static inline u32 mdla_fifo_tail_id(void)
{
	return (last_cmd_id1 == UINT32_MAX) ?
		last_cmd_id0 : last_cmd_id1;
}

static inline bool mdla_fifo_is_empty(void)
{
	return (cmd_list_len == 0);
}

static int mdla_fifo_in(u32 id, struct command_entry *ce)
{
	if (cmd_list_len == 0) {
		mdla_debug("cmd_list_len == 0\n");
		last_cmd_id0 = id;
		last_cmd_id1 = UINT32_MAX;
		last_cmd_ent0 = ce;
		last_cmd_ent1 = NULL;
		cmd_list_len++;
		return 1;
	}

	if (cmd_list_len == 1) {
		mdla_debug("cmd_list_len == 1\n");
		last_cmd_id1 = id;
		last_cmd_ent1 = ce;
		cmd_list_len++;
		return 2;
	}

	mdla_debug("cmd_list_len == %d\n", cmd_list_len);
	return 0;
}
static void mdla_fifo_out(void)
{
	if (!cmd_list_len)
		mdla_debug("%s: MDLA FIFO is already empty\n", __func__);

	cmd_list_len--;
	last_cmd_id0 = last_cmd_id1;
	last_cmd_id1 = UINT32_MAX;
	last_cmd_ent0 = last_cmd_ent1;
	last_cmd_ent1 = NULL;
}

static inline void mdla_fifo_mark_fail(void)
{
	command_entry_result(last_cmd_ent0, MDLA_CMD_TIMEOUT);
	command_entry_result(last_cmd_ent1, MDLA_CMD_TIMEOUT);
}

static inline void mdla_fifo_mark_time(void)
{
	command_entry_end(last_cmd_ent0);
	command_entry_end(last_cmd_ent1);
}
#else
#error "mdla: wrong FIFO size"
#endif

static int mdla_open(struct inode *, struct file *);
static int mdla_release(struct inode *, struct file *);
static long mdla_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static long mdla_compat_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg);
static int mdla_wait_command(struct ioctl_wait_cmd *wt);
static int mdla_mmap(struct file *filp, struct vm_area_struct *vma);
static int mdla_process_command(struct command_entry *ce);
static void mdla_timeup(unsigned long data);

#define MDLA_TIMEOUT_DEFAULT 1000 /* ms */
u32 mdla_timeout = MDLA_TIMEOUT_DEFAULT;
static DEFINE_TIMER(mdla_timer, mdla_timeup, 0, 0);

static const struct file_operations fops = {
	.open = mdla_open,
	.unlocked_ioctl = mdla_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mdla_compat_ioctl,
#endif
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

#define mdla_cfg_set(mask, offset) \
	mdla_cfg_write(mdla_cfg_read(offset) | (mask), (offset))

unsigned int mdla_reg_read(u32 offset)
{
	return ioread32(apu_mdla_cmde_mreg_top + offset);
}

void mdla_reg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_cmde_mreg_top + offset);
}

static const char *reason_str[REASON_MAX+1] = {
	"others",
	"driver_init",
	"command_timeout",
	"power_on",
	"-"
};

static const char *mdla_get_reason_str(int res)
{
	if ((res < 0) || (res > REASON_MAX))
		res = REASON_MAX;

	return reason_str[res];
}

void mdla_reset(int res)
{
	const char *str = mdla_get_reason_str(res);

	pr_info("%s: MDLA RESET: %s(%d)\n", __func__,
		str, res);
	mdla_cfg_write(0x03f, MDLA_SW_RST);
	mdla_cfg_write(0x000, MDLA_SW_RST);
	mdla_cfg_write(0xffffffff, MDLA_CG_CLR);
	mdla_reg_write(MDLA_IRQ_MASK & ~(MDLA_IRQ_SWCMD_DONE),
		MREG_TOP_G_INTP2);

#ifdef CONFIG_MTK_MDLA_ION
	mdla_cfg_set(MDLA_AXI_CTRL_MASK, MDLA_AXI_CTRL);
	mdla_cfg_set(MDLA_AXI_CTRL_MASK, MDLA_AXI1_CTRL);
#endif

	pmu_reset();
	mdla_profile_reset(str);
}

static void mdla_timeup(unsigned long data)
{

	pr_info("%s: MDLA Timeout: %u ms, max_cmd_id: %u, fifo_head_id: %u, fifo_tail_id: %u\n",
		__func__, mdla_timeout, max_cmd_id,
		mdla_fifo_head_id(), mdla_fifo_tail_id());

	mdla_dump_reg();
	mdla_reset(REASON_TIMEOUT);

	mdla_fifo_mark_fail();
	max_cmd_id = mdla_fifo_tail_id();
	mdla_fifo_mark_time();

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
	struct command_entry *ce;
	u64 end;

	mutex_lock(&cmd_list_lock);

	ce = mdla_fifo_head();
	end = ce ? ce->req_end_t : 0;

	mdla_trace_end(max_cmd_id, end, MDLA_TRACE_MODE_INT);

	while (max_cmd_id >= mdla_fifo_head_id()) {
		mdla_trace_end(max_cmd_id, end, MDLA_TRACE_MODE_CMD);
		mdla_dvfs_cmd_end_info(mdla_fifo_head());
		mdla_fifo_out();
		if (!list_empty(&cmd_list)) {
			ce = list_first_entry(&cmd_list,
					struct command_entry, list);
			if (mdla_fifo_in(
				ce->id + ce->count - 1,
				ce)) {
				list_del(&ce->list);
				mdla_process_command(ce);
			} else {
				mdla_debug("%s: MDLA FIFO full\n", __func__);
				break;
			}
		}
	}
	if (mdla_fifo_is_empty() && timer_pending(&mdla_timer)) {
		mdla_profile_stop(1);
		mdla_debug("%s: del_timer().\n", __func__);
		del_timer(&mdla_timer);
#ifdef MTK_MDLA_ALWAYS_POWER_ON
		// do nothing
#else
		mdla_dvfs_cmd_end_shutdown();
#endif
	}
	mutex_unlock(&cmd_list_lock);
	wake_up_interruptible_all(&mdla_cmd_queue);
	mdla_debug(
			"mdla_interrupt max_cmd_id: %d, fifo_head_id: %d, fifo_tail_id: %d\n",
			max_cmd_id, mdla_fifo_head_id(), mdla_fifo_tail_id());
}

static irqreturn_t mdla_interrupt(int irq, void *dev_id)
{
	max_cmd_id = mdla_reg_read(MREG_TOP_G_FIN0);
	mdla_reg_write(MDLA_IRQ_MASK, MREG_TOP_G_INTP0);
	mdla_fifo_mark_time();
	pmu_reg_save();
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
	apu_mdla_gsm_base = (void *) apu_mdla_gsm->start;
	pr_info("%s: apu_mdla_gsm_top: %p, apu_mdla_gsm_base: %p\n",
		__func__, apu_mdla_gsm_top, apu_mdla_gsm_base);

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

	mdla_init_hw(0, pdev);

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

#ifdef CONFIG_FPGA_EARLY_PORTING
	mdla_reset(REASON_DRVINIT);
#endif

	return 0;

}

static int mdla_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	mdla_uninit_hw();
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
#ifdef MTK_MDLA_ALWAYS_POWER_ON
	dbg_power_on = 0;
#endif

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

	INIT_WORK(&mdla_queue, mdla_start_queue);
	mdla_debugfs_init();
	mdla_profile_init();
	pmu_init();

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

static void exec_info_eval(struct command_entry *ce,
	struct ioctl_wait_cmd *wt)
{
	wt->queue_time = ce->req_start_t - ce->queue_t;
	wt->busy_time = ce->req_end_t - ce->req_start_t;
	wt->result = ce->result;
	wt->bandwidth = 0; // TODO: fill bandwidth
}

static int mdla_wait_command(struct ioctl_wait_cmd *wt)
{
	struct command_entry *ce;
	struct ioctl_wait_cmd wc;
	__u32 id = wt->id;
	int ret = 0;

	mdla_debug("%s: enter id:[%d]\n", __func__, id);
	// @suppress("Type cannot be resolved")
	wait_event_interruptible(mdla_cmd_queue, max_cmd_id >= id);

	mutex_lock(&cmd_list_lock);
	while (!list_empty(&cmd_fin_list)) {
		ce = list_first_entry(&cmd_fin_list,
			struct command_entry, list);
		if (max_cmd_id < ce->id)
			break;

		exec_info_eval(ce, &wc);

		mdla_debug("%s: wait_id: %d, id: %u, queue: %llu, start: %llu, end: %llu, wait=%llu, busy=%llu, bandwidth=%u, result=%d\n",
			__func__, id, ce->id,
			(unsigned long long)ce->queue_t,
			(unsigned long long)ce->req_start_t,
			(unsigned long long)ce->req_end_t,
			(unsigned long long)wc.queue_time,
			(unsigned long long)wc.busy_time,
			wc.bandwidth,
			wc.result);

		exec_info_eval(ce, wt);

		if (ce->result != MDLA_CMD_SUCCESS)
			ret = -EIO;

		list_del(&ce->list);
		kfree(ce);
	}
	mutex_unlock(&cmd_list_lock);

	mdla_debug("%s: exit (%s)\n", __func__, ret ? "timeout" : "success");

	return ret;
}

static void mdla_mark_command_id(struct command_entry *ce)
{
	int i;
	u32 count = ce->count;
	u32 *cmd = ce->kva;

	ce->id = cmd_id;

	/* Patch SW command ID */
	for (i = 0; i < count; i++) {
		mdla_debug("%s: add command: %d\n", __func__, cmd_id);
		cmd[(MREG_CMD_SWCMD_ID / sizeof(u32)) +
			(i * (MREG_CMD_SIZE / sizeof(u32)))] = cmd_id++;
	}

	mdla_debug("%s: kva=%p, mva=0x%08x, phys_to_dma=0x%lx\n",
			__func__,
			ce->kva,
			ce->mva,
			(unsigned long)phys_to_dma(mdlactlDevice, ce->mva));

#ifdef CONFIG_MTK_MDLA_ION
	if (ce->khandle) {
		mdla_ion_sync(ce->khandle);
		return;
	}
#endif
	dma_sync_single_for_device(mdlactlDevice,
			phys_to_dma(mdlactlDevice, ce->mva),
			count * 0x180, DMA_TO_DEVICE);
}

static int mdla_process_command(struct command_entry *ce)
{
	dma_addr_t addr = ce->mva;
	u32 count = ce->count;
	u32 *cmd = (u32 *) ce->kva;
	int ret = 0;

	mdla_debug("%s: id: %d, count: %d, addr: %lx\n",
		__func__, cmd[84], ce->count,
		(unsigned long)addr);

#ifdef MTK_MDLA_ALWAYS_POWER_ON
	if (dbg_power_on)
		goto skip_power;
#endif
	ret = mdla_dvfs_cmd_start(ce, &cmd_list);
	if (ret)
		return ret;
#ifdef MTK_MDLA_ALWAYS_POWER_ON
	dbg_power_on = 1;
skip_power:
#endif

	if (mdla_timeout) {
		mdla_debug("%s: mod_timer(), cmd id=%u, [%u]\n",
			__func__, ce->id, cmd[84]);
		mod_timer(&mdla_timer,
			jiffies + msecs_to_jiffies(mdla_timeout));
	}

	mdla_profile_start();
	mdla_trace_begin(count, cmd);
	ce->req_start_t = sched_clock();

	/* Issue command */
	mdla_reg_write(addr, MREG_TOP_G_CDMA1);
	mdla_reg_write(count, MREG_TOP_G_CDMA2);
	mdla_reg_write(ce->id, MREG_TOP_G_CDMA3);

	list_add_tail(&ce->list, &cmd_fin_list);

	return ret;
}

static int mdla_run_command_async(struct ioctl_run_cmd *cd)
{
	u32 id;
	struct command_entry *ce =
		kmalloc(sizeof(struct command_entry), GFP_KERNEL);

	ce->mva = cd->buf.mva + cd->offset;
	mdla_debug("%s: mva=%08x, offset=%08x, count: %u, priority: %u, boost: %u\n",
			__func__,
			cd->buf.mva,
			cd->offset,
			cd->count,
			cd->priority,
			cd->boost_value);

	if (cd->buf.ion_khandle) {
		ce->kva = (void *)cd->buf.kva + cd->offset;
		mdla_debug("%s: <ion> kva=%p, mva=%08x\n",
				__func__,
				ce->kva,
				ce->mva);
	} else if (is_gsm_mva(ce->mva)) {
		ce->kva = gsm_mva_to_virt(ce->mva);
	} else {
		ce->kva = phys_to_virt(ce->mva);
		mdla_debug("%s: <dram> kva: phys_to_virt(mva:%08x) = %p\n",
			__func__, ce->mva, ce->kva);
	}

	ce->count = cd->count;
	ce->khandle = cd->buf.ion_khandle;
	ce->type = cd->buf.type;
	ce->priority = cd->priority;
	ce->boost_value = cd->boost_value;
	ce->bandwidth = 0;
	ce->result = MDLA_CMD_SUCCESS;

	mutex_lock(&cmd_list_lock);

	ce->queue_t = sched_clock();

	/* Increate cmd_id*/
	mdla_mark_command_id(ce);

	id = cmd_id - 1;

	if (mdla_fifo_in(id, ce))
		mdla_process_command(ce);
	else
		list_add_tail(&ce->list, &cmd_list);

	mutex_unlock(&cmd_list_lock);

	return id;
}

static void mdla_dram_alloc(struct ioctl_malloc *malloc_data)
{
	dma_addr_t phyaddr = 0;

	malloc_data->kva = dma_alloc_coherent(mdlactlDevice, malloc_data->size,
			&phyaddr, GFP_KERNEL);
	malloc_data->pa = (void *)dma_to_phys(mdlactlDevice, phyaddr);
	malloc_data->mva = (__u32)((long) malloc_data->pa);

	mdla_debug("%s: kva:%p, mva:%x\n",
		__func__, malloc_data->kva, malloc_data->mva);
}

static void mdla_dram_free(struct ioctl_malloc *malloc_data)
{
	mdla_debug("%s: kva:%p, mva:%x\n",
		__func__, malloc_data->kva, malloc_data->mva);
	dma_free_coherent(mdlactlDevice, malloc_data->size,
			(void *) malloc_data->kva, malloc_data->mva);
}

static long mdla_ioctl(struct file *filp, unsigned int command,
		unsigned long arg)
{
	long retval;
	struct ioctl_malloc malloc_data;
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
		mdla_debug("%s: IOCTL_MALLOC: size:0x%x mva:0x%x kva:%p pa:%p type:%d\n",
			__func__,
			malloc_data.size,
			malloc_data.mva,
			malloc_data.kva,
			malloc_data.pa,
			malloc_data.type);
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
		mdla_debug("%s: IOCTL_MALLOC: size:0x%x mva:0x%x kva:%p pa:%p type:%d\n",
			__func__,
			malloc_data.size,
			malloc_data.mva,
			malloc_data.kva,
			malloc_data.pa,
			malloc_data.type);
		break;
	case IOCTL_RUN_CMD_SYNC:
	{
		struct ioctl_run_cmd_sync cmd_data;

		if (copy_from_user(&cmd_data, (void *) arg,
				sizeof(cmd_data))) {
			retval = -EFAULT;
			return retval;
		}
		mdla_debug("%s: RUN_CMD_SYNC: kva=%p, mva=0x%08x, phys_to_virt=%p\n",
				__func__,
				(void *)cmd_data.req.buf.kva,
				cmd_data.req.buf.mva,
				phys_to_virt(cmd_data.req.buf.mva));
		id = mdla_run_command_async(&cmd_data.req);
		cmd_data.res.id = cmd_data.req.id;
		retval = mdla_wait_command(&cmd_data.res);

		if (copy_to_user((void *) arg, &cmd_data, sizeof(cmd_data)))
			return -EFAULT;

		return retval;
	}
	case IOCTL_RUN_CMD_ASYNC:
	{
		struct ioctl_run_cmd cmd_data;

		if (copy_from_user(&cmd_data, (void *) arg,
				sizeof(cmd_data))) {
			retval = -EFAULT;
			return retval;
		}
		mdla_debug("%s: RUN_CMD_ASYNC: kva=%p, mva=0x%08x, phys_to_virt=%p\n",
				__func__,
				(void *)cmd_data.buf.kva,
				cmd_data.buf.mva,
				phys_to_virt(cmd_data.buf.mva));
		id = mdla_run_command_async(&cmd_data);
		cmd_data.id = id;
		if (copy_to_user((void *) arg, &cmd_data,
			sizeof(cmd_data)))
			return -EFAULT;
		break;
	}
	case IOCTL_WAIT_CMD:
	{
		struct ioctl_wait_cmd wait_data;

		if (copy_from_user(&wait_data, (void *) arg,
				sizeof(wait_data))) {
			retval = -EFAULT;
			return retval;
		}
		retval = mdla_wait_command(&wait_data);
		if (copy_to_user((void *) arg, &wait_data, sizeof(wait_data)))
			return -EFAULT;
		return retval;
	}
	case IOCTL_PERF_SET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.handle = pmu_counter_alloc(
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
		perf_data.event = pmu_counter_event_get(perf_data.handle);
		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;
	case IOCTL_PERF_GET_CNT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		perf_data.counter = pmu_counter_get(perf_data.handle);

		if (copy_to_user((void *) arg, &perf_data, sizeof(perf_data)))
			return -EFAULT;
		break;

	case IOCTL_PERF_UNSET_EVENT:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		pmu_counter_free(perf_data.handle);
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
		pmu_reset_saved_counter();
		break;
	case IOCTL_PERF_RESET_CYCLE:
		pmu_reset_saved_cycle();
		break;
	case IOCTL_PERF_SET_MODE:
		if (copy_from_user(&perf_data, (void *) arg,
				sizeof(perf_data))) {
			retval = -EFAULT;
			return retval;
		}
		pmu_clr_mode_save(perf_data.mode);
		break;
	case IOCTL_ION_KMAP:
		return mdla_ion_kmap(arg);
	case IOCTL_ION_KUNMAP:
		return mdla_ion_kunmap(arg);
	default:
		if (command >= MDLA_DVFS_IOCTL_START &&
			command <= MDLA_DVFS_IOCTL_END)
			return mdla_dvfs_ioctl(filp, command, arg);
		else
			return -EINVAL;
	}
	return 0;
}

#ifdef CONFIG_COMPAT
static long mdla_compat_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	return mdla_ioctl(file, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static void mdlactl_exit(void)
{
	mdla_debugfs_exit();
	platform_driver_unregister(&mdla_driver);
	device_destroy(mdlactlClass, MKDEV(majorNumber, 0));
	class_destroy(mdlactlClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	mdla_debug("MDLA: Goodbye from the LKM!\n");
}

late_initcall(mdlactl_init)
module_exit(mdlactl_exit);

