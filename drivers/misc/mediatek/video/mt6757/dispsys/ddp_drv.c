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
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
/* ION */
/* #include <linux/ion.h> */
/* #include <linux/ion_drv.h> */
/* #include "m4u.h" */
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include "mt-plat/mtk_smi.h"
/* #include <mach/irqs.h> */
/* #include <mach/mt_reg_base.h> */
/* #include <mach/mt_irq.h> */
/* #include <mach/irqs.h> */
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif
/* #include "mach/mt_irq.h" */
#include "mt-plat/sync_write.h"
#include "mt-plat/mtk_smi.h"
#include "m4u.h"

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_hal.h"
#include "ddp_log.h"
#include "ddp_irq.h"
#include "ddp_info.h"
#include "display_recorder.h"

/*#define DISP_NO_DPI*/ /* FIXME: tmp define */
#ifndef DISP_NO_DPI
#include "ddp_dpi_reg.h"
#endif
#include "disp_helper.h"

#define DISP_DEVNAME "DISPSYS"
/* device and driver */
static dev_t disp_devno;
static struct cdev *disp_cdev;
static struct class *disp_class;

struct disp_node_struct {
	pid_t open_pid;
	pid_t open_tgid;
	struct list_head testList;
	spinlock_t node_lock;
};

static struct platform_device mydev;
static struct dispsys_device *dispsys_dev;
unsigned int dispsys_irq[DISP_REG_NUM] = {0};
unsigned long dispsys_reg[DISP_REG_NUM] = {0};

unsigned long mipi_tx0_reg;
unsigned long mipi_tx1_reg;
unsigned long dsi_reg_va[2] = {0};
/* from DTS, should be synced with DTS */
unsigned long ddp_reg_pa_base[DISP_REG_NUM];

#ifndef CONFIG_MTK_CLKMGR
/*
 * Note: The name order of the disp_clk_name[] must be synced with
 * the enum ddp_clk_id(ddp_clkmgr.h) in case get the wrong clock.
 */
const char *disp_clk_name[MAX_DISP_CLK_CNT] = {
	"DISP0_SMI_COMMON",
	"DISP0_SMI_LARB0",
	"DISP0_SMI_LARB4",
	"DISP0_DISP_OVL0",
	"DISP0_DISP_OVL1",
	"DISP0_DISP_OVL0_2L",
	"DISP0_DISP_OVL1_2L",
	"DISP0_DISP_RDMA0",
	"DISP0_DISP_RDMA1",
	"DISP0_DISP_RDMA2",
	"DISP0_DISP_WDMA0",
	"DISP0_DISP_WDMA1",
	"DISP0_DISP_COLOR",
	"DISP0_DISP_COLOR1",
	"DISP0_DISP_CCORR",
	"DISP0_DISP_CCORR1",
	"DISP0_DISP_AAL",
	"DISP0_DISP_AAL1",
	"DISP0_DISP_GAMMA",
	"DISP0_DISP_GAMMA1",
	"DISP0_DISP_OD",
	"DISP0_DISP_DITHER",
	"DISP0_DISP_DITHER1",
	"DISP0_DISP_UFOE",
	"DISP0_DISP_DSC",
	"DISP0_DISP_SPLIT",
	"DISP1_DSI0_MM_CLOCK",
	"DISP1_DSI0_INTERFACE_CLOCK",
	"DISP1_DSI1_MM_CLOCK",
	"DISP1_DSI1_INTERFACE_CLOCK",
	"DISP1_DPI_MM_CLOCK",
	"DISP1_DPI_INTERFACE_CLOCK",
	"DISP1_DISP_OVL0_MOUT",
	"DISP_PWM",
	"DISP_MTCMOS_CLK",
	"MUX_DPI0",
	"TVDPLL_D2",
	"TVDPLL_D4",
	"TVDPLL_D8",
	"TVDPLL_D16",
	"DPI_CK",
	"MUX_PWM",
	"CLK26M",
	"UNIVPLL2_D4",
	"ULPOSC_D4",
	"ULPOSC_D8",
	"MUX_MM",
	"MM_VENCPLL",
	"SYSPLL2_D2",
};

#endif

#if 0 /* defined but not used */
static unsigned int ddp_ms2jiffies(unsigned long ms)
{
	return (ms * HZ + 512) >> 10;
}
#endif

struct dispsys_device {
	void __iomem *regs[DISP_REG_NUM];
	struct device *dev;
	int irq[DISP_REG_NUM];
#ifndef CONFIG_MTK_CLKMGR
	struct clk *disp_clk[MAX_DISP_CLK_CNT];
#endif
};

static int disp_is_intr_enable(enum DISP_REG_ENUM module)
{
	switch (module) {
	case DISP_REG_OVL0:
	case DISP_REG_OVL1:
	case DISP_REG_OVL0_2L:
	case DISP_REG_OVL1_2L:
	case DISP_REG_RDMA0:
	case DISP_REG_RDMA1:
	/*case DISP_REG_WDMA0:*/
	case DISP_REG_WDMA1:
	case DISP_REG_MUTEX:
	case DISP_REG_DSI0:
	case DISP_REG_DPI0:
	case DISP_REG_AAL0:
	case DISP_REG_CCORR0:
		return 1;

	case DISP_REG_COLOR0:
	case DISP_REG_GAMMA0:
	case DISP_REG_DITHER0:
	case DISP_REG_PWM:
	case DISP_REG_CONFIG:
	case DISP_REG_SMI_LARB0:
	case DISP_REG_SMI_COMMON:
	case DISP_REG_MIPI0:
	case DISP_REG_MIPI1:
		return 0;

	case DISP_REG_WDMA0:
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) &&                                   \
	defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
		return 0;
#else
		return 1;
#endif
	default:
		return 0;
	}
}

m4u_callback_ret_t disp_m4u_callback(int port, unsigned long mva, void *data)
{
	enum DISP_MODULE_ENUM module = DISP_MODULE_OVL0;

	DDPERR("fault call port=%d, mva=0x%lx, data=0x%p\n", port, mva, data);
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
	case M4U_PORT_DISP_OVL1:
		module = DISP_MODULE_OVL1;
		break;
	case M4U_PORT_DISP_RDMA1:
		module = DISP_MODULE_RDMA1;
		break;
	case M4U_PORT_DISP_WDMA1:
		module = DISP_MODULE_WDMA1;
		break;
	case M4U_PORT_DISP_2L_OVL0_LARB0:
	case M4U_PORT_DISP_2L_OVL0_LARB4:
		module = DISP_MODULE_OVL0_2L;
		break;
	case M4U_PORT_DISP_2L_OVL1:
		module = DISP_MODULE_OVL1_2L;
		break;
	default:
		DDPERR("unknown port=%d\n", port);
	}
	ddp_dump_analysis(module);
	ddp_dump_reg(module);

	return 0;
}

struct device *disp_get_device(void) { return dispsys_dev->dev; }

static int disp_probe(struct platform_device *pdev)
{
	struct class_device;
	/* int i; */
	static unsigned int disp_probe_cnt;
#ifdef ENABLE_CLK_MGR
	int i;
#endif

	if (disp_probe_cnt != 0)
		return 0;

	/* save pdev for disp_probe_1 */
	memcpy(&mydev, pdev, sizeof(mydev));

	if (dispsys_dev) {
		DDPERR("%s: dispsys_dev=0x%p\n", __func__, dispsys_dev);
		ASSERT(0);
	}

	dispsys_dev = kmalloc(sizeof(struct dispsys_device), GFP_KERNEL);
	if (!dispsys_dev) {
		DDPERR("Unable to allocate dispsys_dev\n");
		return -ENOMEM;
	}

#ifdef ENABLE_CLK_MGR
#ifndef CONFIG_MTK_CLKMGR
	for (i = 0; i < MAX_DISP_CLK_CNT; i++) {
		DDPMSG("DISPSYS get clock %s\n", disp_clk_name[i]);
		dispsys_dev->disp_clk[i] =
		    devm_clk_get(&pdev->dev, disp_clk_name[i]);
		if (IS_ERR(dispsys_dev->disp_clk[i]))
			DDPERR("%s:%d, DISPSYS get %d,%s clock error!!!\n",
			       __FILE__, __LINE__, i, disp_clk_name[i]);
		else {
			if (!ddp_set_clk_handle(dispsys_dev->disp_clk[i], i)) {
				switch (i) {
				case MUX_MM:
				case MM_VENCPLL:
				case SYSPLL2_D2:
					break;

				case DISP0_SMI_COMMON:
				case DISP0_SMI_LARB0:
				case DISP0_SMI_LARB4:
					ddp_clk_prepare_enable(i);
					break;

				case DISP_MTCMOS_CLK:
#ifndef CONFIG_FPGA_EARLY_PORTING
					ddp_clk_prepare_enable(i);
#endif
					break;

				default:
					ddp_clk_prepare(i);
					break;
				}
			}
		}
	}
#endif /* CONFIG_MTK_CLKMGR */
#endif
	disp_probe_cnt++;

	return 0;
}

static int __init disp_probe_1(void)
{
	struct class_device;
	int ret;
	int i;
	struct platform_device *pdev = &mydev;

	disp_init_irq();
	disp_helper_option_init();

	if (!dispsys_dev) {
		DDPERR("%s: dispsys_dev=NULL\n", __func__);
		ASSERT(0);
		return -1;
	}

	dispsys_dev->dev = &pdev->dev;

	/* iomap registers and irq */
	for (i = 0; i < DISP_REG_NUM; i++) {
		struct resource res;

		dispsys_dev->regs[i] = of_iomap(pdev->dev.of_node, i);
		if (!dispsys_dev->regs[i]) {
			DDPERR("Unable to ioremap registers, of_iomap fail, "
			       "i=%d\n",
			       i);
			return -ENOMEM;
		}
		dispsys_reg[i] = (unsigned long)(dispsys_dev->regs[i]);
		/* get physical address */
		of_address_to_resource(pdev->dev.of_node, i, &res);
		ddp_reg_pa_base[i] = res.start;

		/* get IRQ ID and request IRQ */
		dispsys_dev->irq[i] =
		    irq_of_parse_and_map(pdev->dev.of_node, i);
		dispsys_irq[i] = dispsys_dev->irq[i];

		DDPMSG("DT, i=%d, module=%s, map_addr=%p, map_irq=%d, "
		       "reg_pa=0x%lx\n",
		       i, ddp_get_reg_module_name(i), dispsys_dev->regs[i],
		       dispsys_dev->irq[i], ddp_reg_pa_base[i]);
	}
	/* mipi tx reg map here */
	dsi_reg_va[0] = dispsys_reg[DISP_REG_DSI0];
	dsi_reg_va[1] = dispsys_reg[DISP_REG_DSI1];
	mipi_tx0_reg = dispsys_reg[DISP_REG_MIPI0];
	mipi_tx1_reg = dispsys_reg[DISP_REG_MIPI1];
#ifndef DISP_NO_DPI
	DPI_REG = (struct DPI_REGS *)dispsys_reg[DISP_REG_DPI0];
#endif
/* //// power on MMSYS for early porting */
#ifdef CONFIG_FPGA_EARLY_PORTING
	pr_debug("[DISP Probe] power MMSYS:0x%lx,0x%lx\n",
		 DISP_REG_CONFIG_MMSYS_CG_CLR0, DISP_REG_CONFIG_MMSYS_CG_CLR1);
	DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_CG_CLR0, 0xFFFFFFFF);
	DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_CG_CLR1, 0xFFFFFFFF);
/* sel ovl0_2l to larb0 */
/* DISP_REG_SET(NULL, DISP_REG_CONFIG_MMSYS_SMI_LARB_SEL, 0x1);*/
#endif

	/* init arrays */
	ddp_path_init();

	for (i = 0; i < DISP_REG_NUM; i++) {
		if (disp_is_intr_enable(i) == 1) {
			/* IRQF_TRIGGER_NONE dose not take effect here, real
			 * trigger mode set in dts file */
			ret = request_irq(dispsys_dev->irq[i],
					  (irq_handler_t)disp_irq_handler,
					  IRQF_TRIGGER_NONE,
					  ddp_get_reg_module_name(i), NULL);
			if (ret) {
				DDPERR("Unable to request IRQ, request_irq "
				       "fail, i=%d, irq=%d\n",
				       i, dispsys_dev->irq[i]);
				return ret;
			}
			DDPMSG("irq enabled, module=%s, irq=%d\n",
			       ddp_get_reg_module_name(i), dispsys_dev->irq[i]);
		}
	}

	/* init M4U callback */
	DDPMSG("register m4u callback\n");
	m4u_register_fault_callback(
	    M4U_PORT_DISP_OVL0, (m4u_fault_callback_t *)disp_m4u_callback, 0);
	m4u_register_fault_callback(
	    M4U_PORT_DISP_RDMA0, (m4u_fault_callback_t *)disp_m4u_callback, 0);
	m4u_register_fault_callback(
	    M4U_PORT_DISP_WDMA0, (m4u_fault_callback_t *)disp_m4u_callback, 0);
	m4u_register_fault_callback(
	    M4U_PORT_DISP_OVL1, (m4u_fault_callback_t *)disp_m4u_callback, 0);
	m4u_register_fault_callback(
	    M4U_PORT_DISP_RDMA1, (m4u_fault_callback_t *)disp_m4u_callback, 0);
	m4u_register_fault_callback(
	    M4U_PORT_DISP_WDMA1, (m4u_fault_callback_t *)disp_m4u_callback, 0);
	m4u_register_fault_callback(M4U_PORT_DISP_2L_OVL0_LARB4,
				    (m4u_fault_callback_t *)disp_m4u_callback,
				    0);
	m4u_register_fault_callback(M4U_PORT_DISP_2L_OVL0_LARB0,
				    (m4u_fault_callback_t *)disp_m4u_callback,
				    0);
	m4u_register_fault_callback(M4U_PORT_DISP_2L_OVL1,
				    (m4u_fault_callback_t *)disp_m4u_callback,
				    0);

	DDPMSG("dispsys probe done.\n");
	/* NOT_REFERENCED(class_dev); */
	return 0;
}

static int disp_remove(struct platform_device *pdev) { return 0; }

static void disp_shutdown(struct platform_device *pdev) { /* Nothing yet */ }

/* PM suspend */
static int disp_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

/* PM resume */
static int disp_resume(struct platform_device *pdev) { return 0; }

static const struct of_device_id dispsys_of_ids[] = {
	{
	.compatible = "mediatek,dispsys",
	},
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

	init_log_buffer();
	DDPMSG("Register the disp driver\n");
	if (platform_driver_register(&dispsys_of_driver)) {
		DDPERR("failed to register disp driver\n");
		/* platform_device_unregister(&disp_device); */
		ret = -ENODEV;
		return ret;
	}
	DDPMSG("disp driver init done\n");
	return 0;
}

static void __exit disp_exit(void)
{
#ifndef CONFIG_MTK_CLKMGR
	int i = 0;

	for (i = 0; i < MAX_DISP_CLK_CNT; i++)
		ddp_clk_unprepare(i);
#endif
	cdev_del(disp_cdev);
	unregister_chrdev_region(disp_devno, 1);

	platform_driver_unregister(&dispsys_of_driver);

	device_destroy(disp_class, disp_devno);
	class_destroy(disp_class);
}

#ifndef MTK_FB_DO_NOTHING
arch_initcall(disp_init);
module_init(disp_probe_1);
module_exit(disp_exit);
#endif
MODULE_AUTHOR("Tzu-Meng, Chung <Tzu-Meng.Chung@mediatek.com>");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");
