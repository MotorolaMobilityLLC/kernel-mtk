/*
 * Copyright (C) 2015 MediaTek Inc.
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

#define LOG_TAG "ddp_drv"

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/timer.h>
/* #include <mach/mt_smi.h> */
#include <linux/proc_fs.h>	/* proc file use */
#include <linux/miscdevice.h>
/* ION */
/* #include <linux/ion.h> */
/* #include <linux/ion_drv.h> */
/* #include <mach/m4u.h> */
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
/* #include <mach/mt_smi.h> */
/* #include <mach/irqs.h> */
/* #include <mach/mt_reg_base.h> */
/* #include <mach/mt_irq.h> */
/* #include <mach/irqs.h> */
#include <mach/mt_clkmgr.h>
#include <mt-plat/sync_write.h>
#include "m4u.h"

#include "disp_drv_platform.h"
#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_hal.h"
#include "ddp_irq.h"
#include "ddp_info.h"
/* #include "ddp_dpi_reg.h" */
#include "ddp_path.h"
#include "disp_debug.h"
#include "mtkfb_debug.h"
#include "disp_log.h"

#define DISP_DEVNAME "DISPSYS"
/* device and driver */
static dev_t disp_devno;
static struct cdev *disp_cdev;
static struct class *disp_class;

typedef struct {
	pid_t open_pid;
	pid_t open_tgid;
	struct list_head testList;
	spinlock_t node_lock;
} disp_node_struct;


static struct kobject kdispobj;
/*****************************************************************************/
/*****************************************************************************/
/* sysfs for access information */
/* --------------------------------------// */
static ssize_t disp_kobj_show(struct kobject *kobj, struct attribute *attr, char *buffer)
{
	int size = 0x0;

	if (0 == strcmp(attr->name, "dbg1"))
		size = ddp_dump_reg_to_buf(2, (unsigned long *)buffer);
	else if (0 == strcmp(attr->name, "dbg2"))
		size = ddp_dump_reg_to_buf(1, (unsigned long *)buffer);
	else if (0 == strcmp(attr->name, "dbg3"))
		size = ddp_dump_reg_to_buf(0, (unsigned long *)buffer);

	return size;
}

/* --------------------------------------// */

static struct kobj_type disp_kobj_ktype = {
	.sysfs_ops = &(const struct sysfs_ops){
					 .show = disp_kobj_show,
					 .store = NULL},
	.default_attrs = (struct attribute *[]){
						&(struct attribute){
								    .name = "dbg1",	/* disp, dbg1 */
								    .mode = S_IRUGO},
						&(struct attribute){
								    .name = "dbg2",	/* disp, dbg2 */
								    .mode = S_IRUGO},
						&(struct attribute){
								    .name = "dbg3",	/* disp, dbg3 */
								    .mode = S_IRUGO},
						NULL}
};
static long disp_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	/* disp_node_struct *pNode = (disp_node_struct *) file->private_data; */

	return 0;
}

static int disp_open(struct inode *inode, struct file *file)
{
	disp_node_struct *pNode = NULL;

	DISPDBG("enter disp_open() process:%s\n", current->comm);

	/* Allocate and initialize private data */
	file->private_data = kmalloc(sizeof(disp_node_struct), GFP_ATOMIC);
	if (NULL == file->private_data) {
		DISPERR("Not enough entry for DISP open operation\n");
		return -ENOMEM;
	}

	pNode = (disp_node_struct *) file->private_data;
	pNode->open_pid = current->pid;
	pNode->open_tgid = current->tgid;
	INIT_LIST_HEAD(&(pNode->testList));
	spin_lock_init(&pNode->node_lock);

	return 0;

}

static ssize_t disp_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
	return 0;
}

static int disp_release(struct inode *inode, struct file *file)
{
	disp_node_struct *pNode = NULL;
	/* unsigned int index = 0; */

	DISPDBG("enter disp_release() process:%s\n", current->comm);

	pNode = (disp_node_struct *) file->private_data;

	spin_lock(&pNode->node_lock);

	spin_unlock(&pNode->node_lock);

	if (NULL != file->private_data) {
		kfree(file->private_data);
		file->private_data = NULL;
	}

	return 0;
}

static int disp_flush(struct file *file, fl_owner_t a_id)
{
	return 0;
}

/* remap register to user space */
static int disp_mmap(struct file *file, struct vm_area_struct *a_pstVMArea)
{
	return 0;
}

struct dispsys_device {
	void __iomem *regs[DISP_REG_NUM];
	struct device *dev;
	int irq[DISP_REG_NUM];
};
static struct dispsys_device *dispsys_dev;
static int nr_dispsys_dev;
unsigned int dispsys_irq[DISP_REG_NUM] = { 0 };
volatile unsigned long dispsys_reg[DISP_REG_NUM] = { 0 };

volatile unsigned long mipi_tx_reg = 0;
volatile unsigned long dsi_reg_va = 0;

/* extern irqreturn_t disp_irq_handler(int irq, void *dev_id); */
/* extern int ddp_path_init(void); */
/* from DTS, for debug */
unsigned long ddp_reg_pa_base[DISP_REG_NUM] = {
	0x14000000, 0x14007000, 0x14008000, 0x14009000,
	0x1400A000, 0x1400B000, 0x1400C000, 0x1400D000,
	0x1400E000, 0x1400F000, 0x14010000, 0x14011000,
	0x14012000, 0x14013000, 0x1100F000, 0x14015000,
	0x14016000, 0x14017000, 0x14018000
};

unsigned int ddp_irq_num[DISP_REG_NUM] = {
	0, 119, 0, 121,
	0, 123, 124, 0,
	126, 127, 128, 0,
	130, 0, 0, 112,
	0, 0, 0
};

static int disp_is_intr_enable(DISP_REG_ENUM module)
{
	switch (module) {
	case DISP_REG_OVL0:
	case DISP_REG_RDMA0:
	case DISP_REG_WDMA0:
	case DISP_REG_MUTEX:
	case DISP_REG_DSI0:
	case DISP_REG_AAL:
		return 1;
	case DISP_REG_RDMA1:
	case DISP_REG_COLOR:
	case DISP_REG_CCORR:
	case DISP_REG_GAMMA:
	case DISP_REG_DITHER:
	case DISP_REG_PWM:
	case DISP_REG_DPI0:
	case DISP_REG_CONFIG:
	case DISP_REG_SMI_LARB0:
	case DISP_REG_SMI_COMMON:
	case DISP_REG_MIPI:
		return 0;
	default:
		return 0;
	}
}

m4u_callback_ret_t disp_m4u_callback(int port, unsigned int mva, void *data)
{
	DISP_MODULE_ENUM module = DISP_MODULE_OVL0;

	DISPERR("fault call port=%d, mva=0x%x, data=0x%p\n", port, mva, data);
	switch (port) {
	case M4U_PORT_DISP_OVL0:
		module = DISP_MODULE_OVL0;
		break;
	case M4U_PORT_DISP_RDMA0:
		module = DISP_MODULE_RDMA0;
		break;
	case M4U_PORT_DISP_WDMA0:
		module = DISP_MODULE_WDMA0;
		break;
	default:
		DISPERR("unknown port=%d\n", port);
	}
	ddp_dump_analysis(module);
	ddp_dump_reg(module);

	return 0;
}

static int disp_probe(struct platform_device *pdev)
{
	struct class_device;
	int ret;
	int i;
	int new_count;
	static unsigned int disp_probe_cnt;

	if (disp_probe_cnt != 0)
		return 0;

	new_count = nr_dispsys_dev + 1;
	/* dispsys_dev = krealloc(dispsys_dev, sizeof(struct dispsys_device) * new_count, GFP_KERNEL); */
	dispsys_dev = kcalloc(new_count, sizeof(*dispsys_dev), GFP_KERNEL);
	if (!dispsys_dev) {
		DISPERR("Unable to allocate dispsys_dev\n");
		return -ENOMEM;
	}

	dispsys_dev = &(dispsys_dev[nr_dispsys_dev]);
	dispsys_dev->dev = &pdev->dev;

	/* iomap registers and irq */
	for (i = 0; i < DISP_REG_NUM; i++) {
		dispsys_dev->regs[i] = of_iomap(pdev->dev.of_node, i);
		if (!dispsys_dev->regs[i]) {
			DISPERR("Unable to ioremap registers, of_iomap fail, i=%d\n", i);
			return -ENOMEM;
		}
		dispsys_reg[i] = (unsigned long)dispsys_dev->regs[i];
		/* Move to here for outof for_loop, due to it will be needed soon after request_irq */
		if (i == DISP_REG_DSI0)
			dsi_reg_va = dispsys_reg[DISP_REG_DSI0];
		if (i == DISP_REG_MIPI)
			mipi_tx_reg = dispsys_reg[DISP_REG_MIPI];

		/* get IRQ ID and request IRQ */
		dispsys_dev->irq[i] = irq_of_parse_and_map(pdev->dev.of_node, i);
		dispsys_irq[i] = dispsys_dev->irq[i];
		if (!dispsys_irq[i])
			continue;

		if (disp_is_intr_enable(i) == 1) {
			/* IRQF_TRIGGER_NONE dose not take effect here, real trigger mode set in dts file */
			ret = request_irq(dispsys_dev->irq[i], (irq_handler_t) disp_irq_handler,
					  IRQF_TRIGGER_NONE, DISP_DEVNAME, NULL);

			if (ret) {
				DISPERR("Unable to request IRQ, request_irq fail, i=%d, irq=%d\n", i,
				       dispsys_dev->irq[i]);
				return ret;
			}
		}
		DISPDBG("DT, i=%d, module=%s, map_addr=%p, map_irq=%d, reg_pa=0x%lx, irq=%d\n",
		       i, ddp_get_reg_module_name(i), dispsys_dev->regs[i], dispsys_dev->irq[i],
		       ddp_reg_pa_base[i], ddp_irq_num[i]);
	}
	nr_dispsys_dev = new_count;
	/* mipi tx reg map here */
	/* dsi_reg_va = dispsys_reg[DISP_REG_DSI0]; */
	/* mipi_tx_reg = dispsys_reg[DISP_REG_MIPI]; */

	/* power on MMSYS for early porting */
#ifdef CONFIG_MTK_FPGA
	DISPDBG("[DISP Probe] power MMSYS:0x%lx,0x%lx\n", DISP_REG_CONFIG_MMSYS_CG_CLR0,
	       DISP_REG_CONFIG_MMSYS_CG_CLR1);
	DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_CG_CLR0, 0xFFFFFFFF);
	DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_CG_CLR1, 0xFFFFFFFF);
	/* DISP_REG_SET(NULL,DISPSYS_CONFIG_BASE + 0xC04,0x1C000); *//* fpga should set this register */
#endif

	/* init arrays */
	ddp_path_init();

	/* init M4U callback */
	DISPDBG("register m4u callback\n");
	m4u_register_fault_callback(M4U_PORT_DISP_OVL0, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_RDMA0, disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_WDMA0, disp_m4u_callback, 0);
	/* m4u_register_fault_callback(M4U_PORT_DISP_OVL1, disp_m4u_callback, 0); */
	/* m4u_register_fault_callback(M4U_PORT_DISP_RDMA1, disp_m4u_callback, 0); */
	/* m4u_register_fault_callback(M4U_PORT_DISP_WDMA1, disp_m4u_callback, 0); */

	/* sysfs */
	DISPDBG("sysfs disp +");
	/* add kobject */
	if (kobject_init_and_add(&kdispobj, &disp_kobj_ktype, NULL, "disp") < 0) {
		DISPDBG("fail to add disp\n");
		return -ENOMEM;
	}

	DISPDBG("dispsys probe done.\n");
	/* NOT_REFERENCED(class_dev); */
	return 0;
}

static int disp_remove(struct platform_device *pdev)
{
	return 0;
}

static void disp_shutdown(struct platform_device *pdev)
{
	/* Nothing yet */
}


/* PM suspend */
static int disp_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	DISPDBG("\n\n==== DISP suspend is called ====\n");

	return 0;
}

/* PM resume */
static int disp_resume(struct platform_device *pdev)
{
	DISPDBG("\n\n==== DISP resume is called ====\n");

	return 0;
}

/* Kernel interface */
static const struct file_operations disp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = disp_unlocked_ioctl,
	.open = disp_open,
	.release = disp_release,
	.flush = disp_flush,
	.read = disp_read,
	.mmap = disp_mmap
};

static const struct of_device_id dispsys_of_ids[] = {
	{.compatible = "mediatek,DISPSYS",},
	{}
};

static struct platform_driver dispsys_of_driver = {
	.driver = {
		   .name = DISP_DEVNAME,
		   .owner = THIS_MODULE,
		   .of_match_table = dispsys_of_ids,
		   },
	.probe = disp_probe,
	.remove = disp_remove,
	.shutdown = disp_shutdown,
	.suspend = disp_suspend,
	.resume = disp_resume,
};

static int __init disp_init(void)
{
	int ret = 0;

	DISPDBG("Register the disp driver\n");
	if (platform_driver_register(&dispsys_of_driver)) {
		DISPERR("failed to register disp driver\n");
		/* platform_device_unregister(&disp_device); */
		ret = -ENODEV;
		return ret;
	}

	return 0;
}

static void __exit disp_exit(void)
{
	cdev_del(disp_cdev);
	unregister_chrdev_region(disp_devno, 1);

	platform_driver_unregister(&dispsys_of_driver);

	device_destroy(disp_class, disp_devno);
	class_destroy(disp_class);

}

static int __init disp_init_debug_buffer(void)
{
	pr_warn("Init the disp log buffer\n");
	init_log_buffer();

	return 0;
}

#ifndef MTK_FB_DO_NOTHING
arch_initcall(disp_init_debug_buffer);
module_init(disp_init);
module_exit(disp_exit);
#endif
MODULE_AUTHOR("Tzu-Meng, Chung <Tzu-Meng.Chung@mediatek.com>");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");
